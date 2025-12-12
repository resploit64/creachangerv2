#include "pattern_scanner.h"
#include <Psapi.h>
#include <fstream>
#include <sstream>
#include <iomanip>
#include <cstdio>

namespace PatternScanner {

    // ============================================================================
    // Funções Básicas
    // ============================================================================

    uintptr_t FindPattern(const char* moduleName, const uint8_t* pattern, const char* mask) {
        HMODULE hModule = GetModuleHandleA(moduleName);
        if (!hModule) {
            printf("[PatternScanner] Módulo '%s' não encontrado\n", moduleName);
            return 0;
        }

        MODULEINFO moduleInfo;
        if (!GetModuleInformation(GetCurrentProcess(), hModule, &moduleInfo, sizeof(MODULEINFO))) {
            printf("[PatternScanner] Falha ao obter informações do módulo '%s'\n", moduleName);
            return 0;
        }

        uintptr_t moduleBase = (uintptr_t)hModule;
        size_t moduleSize = moduleInfo.SizeOfImage;
        size_t patternLength = strlen(mask);

        for (size_t i = 0; i < moduleSize - patternLength; i++) {
            bool found = true;
            for (size_t j = 0; j < patternLength; j++) {
                if (mask[j] != '?' && pattern[j] != *(uint8_t*)(moduleBase + i + j)) {
                    found = false;
                    break;
                }
            }

            if (found) {
                printf("[PatternScanner] Pattern encontrado em: 0x%p (offset: 0x%llX)\n", 
                       (void*)(moduleBase + i), i);
                return moduleBase + i;
            }
        }

        printf("[PatternScanner] Pattern não encontrado em '%s'\n", moduleName);
        return 0;
    }

    bool ParsePatternString(const char* patternStr, std::vector<uint8_t>& outBytes, std::string& outMask) {
        outBytes.clear();
        outMask.clear();

        std::istringstream iss(patternStr);
        std::string token;

        while (iss >> token) {
            if (token == "??" || token == "?") {
                outBytes.push_back(0);
                outMask += '?';
            } else {
                try {
                    uint8_t byte = (uint8_t)std::stoul(token, nullptr, 16);
                    outBytes.push_back(byte);
                    outMask += 'x';
                } catch (...) {
                    printf("[PatternScanner] Erro ao parsear token: %s\n", token.c_str());
                    return false;
                }
            }
        }

        return !outBytes.empty();
    }

    uintptr_t FindPatternString(const char* moduleName, const char* patternStr) {
        std::vector<uint8_t> bytes;
        std::string mask;

        if (!ParsePatternString(patternStr, bytes, mask)) {
            printf("[PatternScanner] Falha ao parsear pattern string\n");
            return 0;
        }

        return FindPattern(moduleName, bytes.data(), mask.c_str());
    }

    std::vector<uintptr_t> FindAllPatterns(const char* moduleName, const uint8_t* pattern, const char* mask, size_t maxResults) {
        std::vector<uintptr_t> results;

        HMODULE hModule = GetModuleHandleA(moduleName);
        if (!hModule) return results;

        MODULEINFO moduleInfo;
        if (!GetModuleInformation(GetCurrentProcess(), hModule, &moduleInfo, sizeof(MODULEINFO))) {
            return results;
        }

        uintptr_t moduleBase = (uintptr_t)hModule;
        size_t moduleSize = moduleInfo.SizeOfImage;
        size_t patternLength = strlen(mask);

        for (size_t i = 0; i < moduleSize - patternLength; i++) {
            bool found = true;
            for (size_t j = 0; j < patternLength; j++) {
                if (mask[j] != '?' && pattern[j] != *(uint8_t*)(moduleBase + i + j)) {
                    found = false;
                    break;
                }
            }

            if (found) {
                results.push_back(moduleBase + i);
                if (maxResults > 0 && results.size() >= maxResults) {
                    break;
                }
            }
        }

        printf("[PatternScanner] Encontradas %zu ocorrências do pattern\n", results.size());
        return results;
    }

    uint32_t ExtractOffset(uintptr_t instructionAddress, int offsetPosition) {
        if (instructionAddress == 0) return 0;
        return *(uint32_t*)(instructionAddress + offsetPosition);
    }

    uintptr_t ResolveRelativeAddress(uintptr_t instructionAddress, int instructionSize, int offsetPosition) {
        if (instructionAddress == 0) return 0;

        int32_t relativeOffset = *(int32_t*)(instructionAddress + offsetPosition);
        return instructionAddress + instructionSize + relativeOffset;
    }

    bool GetModuleInfo(const char* moduleName, uintptr_t* outBase, size_t* outSize) {
        HMODULE hModule = GetModuleHandleA(moduleName);
        if (!hModule) return false;

        MODULEINFO moduleInfo;
        if (!GetModuleInformation(GetCurrentProcess(), hModule, &moduleInfo, sizeof(MODULEINFO))) {
            return false;
        }

        if (outBase) *outBase = (uintptr_t)hModule;
        if (outSize) *outSize = moduleInfo.SizeOfImage;

        return true;
    }

    // ============================================================================
    // PatternCache
    // ============================================================================

    PatternCache::PatternCache() : m_initialized(false) {}

    PatternCache::~PatternCache() {
        Clear();
    }

    void PatternCache::AddPattern(const std::string& name, const std::vector<uint8_t>& bytes, const std::string& mask, int offsetPos) {
        m_patterns.emplace_back(name, bytes, mask, offsetPos);
    }

    bool PatternCache::AddPatternString(const std::string& name, const char* patternStr, int offsetPos) {
        std::vector<uint8_t> bytes;
        std::string mask;

        if (!ParsePatternString(patternStr, bytes, mask)) {
            return false;
        }

        AddPattern(name, bytes, mask, offsetPos);
        return true;
    }

    bool PatternCache::ScanAll(const char* moduleName) {
        printf("\n[PatternCache] Iniciando scan de %zu patterns em '%s'...\n", m_patterns.size(), moduleName);

        m_cache.clear();
        bool allFound = true;

        for (const auto& pattern : m_patterns) {
            printf("[PatternCache] Escaneando: %s\n", pattern.name.c_str());

            ScanResult result;
            result.patternName = pattern.name;
            result.address = FindPattern(moduleName, pattern.bytes.data(), pattern.mask.c_str());
            result.found = (result.address != 0);

            if (result.found && pattern.offsetPosition >= 0) {
                result.offset = ExtractOffset(result.address, pattern.offsetPosition);
                printf("  → Offset extraído: 0x%X\n", result.offset);
            }

            m_cache.push_back(result);

            if (!result.found) {
                printf("  ✗ NÃO ENCONTRADO\n");
                allFound = false;
            } else {
                printf("  ✓ Encontrado\n");
            }
        }

        m_initialized = true;
        printf("\n[PatternCache] Scan completo. Sucesso: %d/%zu\n", 
               (int)std::count_if(m_cache.begin(), m_cache.end(), [](const ScanResult& r) { return r.found; }),
               m_cache.size());

        return allFound;
    }

    std::optional<ScanResult> PatternCache::GetResult(const std::string& name) const {
        for (const auto& result : m_cache) {
            if (result.patternName == name) {
                return result;
            }
        }
        return std::nullopt;
    }

    std::optional<uint32_t> PatternCache::GetOffset(const std::string& name) const {
        auto result = GetResult(name);
        if (result && result->found) {
            return result->offset;
        }
        return std::nullopt;
    }

    bool PatternCache::SaveToFile(const char* filename) const {
        std::ofstream file(filename);
        if (!file.is_open()) {
            printf("[PatternCache] Falha ao abrir arquivo: %s\n", filename);
            return false;
        }

        file << "{\n";
        for (size_t i = 0; i < m_cache.size(); i++) {
            const auto& result = m_cache[i];
            file << "  \"" << result.patternName << "\": {\n";
            file << "    \"found\": " << (result.found ? "true" : "false") << ",\n";
            file << "    \"address\": \"0x" << std::hex << result.address << "\",\n";
            file << "    \"offset\": \"0x" << std::hex << result.offset << "\"\n";
            file << "  }";
            if (i < m_cache.size() - 1) file << ",";
            file << "\n";
        }
        file << "}\n";

        file.close();
        printf("[PatternCache] Resultados salvos em: %s\n", filename);
        return true;
    }

    bool PatternCache::LoadFromFile(const char* filename) {
        // TODO: Implementar parsing JSON
        // Por simplicidade, não implementado nesta versão
        printf("[PatternCache] LoadFromFile não implementado\n");
        return false;
    }

    bool PatternCache::IsComplete() const {
        return m_initialized && std::all_of(m_cache.begin(), m_cache.end(), 
                                            [](const ScanResult& r) { return r.found; });
    }

    void PatternCache::Clear() {
        m_patterns.clear();
        m_cache.clear();
        m_initialized = false;
    }

    void PatternCache::PrintStats() const {
        printf("\n=== PatternCache Stats ===\n");
        printf("Total patterns: %zu\n", m_patterns.size());
        printf("Patterns encontrados: %zu\n", 
               std::count_if(m_cache.begin(), m_cache.end(), [](const ScanResult& r) { return r.found; }));
        printf("Patterns faltantes: %zu\n",
               std::count_if(m_cache.begin(), m_cache.end(), [](const ScanResult& r) { return !r.found; }));

        if (!m_cache.empty()) {
            printf("\nDetalhes:\n");
            for (const auto& result : m_cache) {
                printf("  [%s] %s\n", result.found ? "✓" : "✗", result.patternName.c_str());
                if (result.found) {
                    printf("    Address: 0x%p, Offset: 0x%X\n", (void*)result.address, result.offset);
                }
            }
        }
        printf("=========================\n\n");
    }

    // ============================================================================
    // Patterns Pré-definidos para CS2
    // ============================================================================

    void InitializeCS2Patterns(PatternCache& cache) {
        printf("[PatternScanner] Inicializando patterns padrão para CS2...\n");

        // m_pGCClientSystem em CGCClient
        cache.AddPatternString("CGCClient::m_pGCClientSystem", GCPatterns::GET_GC_CLIENT_SYSTEM, 3);

        // m_pSharedObjectCache em CGCClientSystem
        cache.AddPatternString("CGCClientSystem::m_pSharedObjectCache", GCPatterns::GET_SHARED_OBJECT_CACHE, 3);

        // m_vecObjects em CGCClientSharedObjectTypeCache
        cache.AddPatternString("CGCClientSharedObjectTypeCache::m_vecObjects", GCPatterns::GET_OBJECTS_VECTOR, 3);

        // m_pItemDefinition em CEconItem
        cache.AddPatternString("CEconItem::m_pItemDefinition", GCPatterns::GET_ITEM_DEFINITION, 3);

        // Attributes em CAttributeList
        cache.AddPatternString("CAttributeList::m_pAttributes", GCPatterns::GET_ATTRIBUTE_BY_INDEX, 3);

        // Adicione mais patterns conforme necessário
        // cache.AddPatternString("Outro::Offset", "48 8B ?? ?? ?? 85", 2);

        printf("[PatternScanner] %zu patterns registrados\n", cache.m_patterns.size());
    }

} // namespace PatternScanner
