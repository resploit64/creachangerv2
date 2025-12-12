#pragma once
#include <windows.h>
#include <cstdint>
#include <vector>
#include <string>
#include <optional>

namespace PatternScanner {

    /**
     * Estrutura para armazenar informações de um pattern
     */
    struct Pattern {
        std::string name;
        std::vector<uint8_t> bytes;
        std::string mask;
        int offsetPosition;  // Posição do offset dentro do pattern (-1 se não aplicável)
        
        Pattern(const std::string& n, const std::vector<uint8_t>& b, const std::string& m, int pos = -1)
            : name(n), bytes(b), mask(m), offsetPosition(pos) {}
    };

    /**
     * Resultado de uma busca de pattern
     */
    struct ScanResult {
        bool found;
        uintptr_t address;
        uint32_t offset;  // Offset extraído (se aplicável)
        std::string patternName;
        
        ScanResult() : found(false), address(0), offset(0), patternName("") {}
    };

    /**
     * Encontra um pattern de bytes em um módulo
     * @param moduleName Nome do módulo (ex: "client.dll")
     * @param pattern Array de bytes para buscar
     * @param mask Máscara (ex: "xxx????xxx" onde ? = wildcard)
     * @return Endereço onde o pattern foi encontrado, ou 0 se não encontrado
     */
    uintptr_t FindPattern(const char* moduleName, const uint8_t* pattern, const char* mask);

    /**
     * Versão sobrecarregada que aceita string hexadecimal
     * @param moduleName Nome do módulo
     * @param patternStr String hexadecimal (ex: "48 8B 81 ?? ?? ?? ?? 48 85 C0")
     * @return Endereço encontrado ou 0
     */
    uintptr_t FindPatternString(const char* moduleName, const char* patternStr);

    /**
     * Encontra múltiplas ocorrências de um pattern
     * @param moduleName Nome do módulo
     * @param pattern Array de bytes
     * @param mask Máscara
     * @param maxResults Número máximo de resultados (0 = ilimitado)
     * @return Vector com endereços encontrados
     */
    std::vector<uintptr_t> FindAllPatterns(const char* moduleName, const uint8_t* pattern, const char* mask, size_t maxResults = 0);

    /**
     * Extrai um offset de 4 bytes de uma instrução
     * @param instructionAddress Endereço da instrução
     * @param offsetPosition Posição do offset dentro da instrução
     * @return Offset extraído
     */
    uint32_t ExtractOffset(uintptr_t instructionAddress, int offsetPosition);

    /**
     * Resolve um endereço relativo RIP (x64)
     * @param instructionAddress Endereço da instrução
     * @param instructionSize Tamanho total da instrução
     * @param offsetPosition Posição do offset dentro da instrução
     * @return Endereço absoluto resolvido
     */
    uintptr_t ResolveRelativeAddress(uintptr_t instructionAddress, int instructionSize, int offsetPosition);

    /**
     * Converte string hexadecimal em bytes e máscara
     * @param patternStr String hexadecimal (ex: "48 8B ?? ?? 85")
     * @param outBytes Vector para armazenar bytes
     * @param outMask String para armazenar máscara
     * @return true se conversão foi bem-sucedida
     */
    bool ParsePatternString(const char* patternStr, std::vector<uint8_t>& outBytes, std::string& outMask);

    /**
     * Obtém informações sobre um módulo
     * @param moduleName Nome do módulo
     * @param outBase Ponteiro para armazenar endereço base
     * @param outSize Ponteiro para armazenar tamanho
     * @return true se módulo foi encontrado
     */
    bool GetModuleInfo(const char* moduleName, uintptr_t* outBase, size_t* outSize);

    /**
     * Classe para gerenciar patterns conhecidos e fazer cache de resultados
     */
    class PatternCache {
    private:
        std::vector<Pattern> m_patterns;
        std::vector<ScanResult> m_cache;
        bool m_initialized;

    public:
        PatternCache();
        ~PatternCache();

        /**
         * Adiciona um pattern ao cache
         */
        void AddPattern(const std::string& name, const std::vector<uint8_t>& bytes, const std::string& mask, int offsetPos = -1);

        /**
         * Adiciona um pattern usando string hexadecimal
         */
        bool AddPatternString(const std::string& name, const char* patternStr, int offsetPos = -1);

        /**
         * Escaneia todos os patterns registrados
         * @param moduleName Nome do módulo para escanear
         * @return true se todos os patterns foram encontrados
         */
        bool ScanAll(const char* moduleName);

        /**
         * Obtém resultado de um pattern pelo nome
         */
        std::optional<ScanResult> GetResult(const std::string& name) const;

        /**
         * Obtém offset descoberto pelo nome do pattern
         */
        std::optional<uint32_t> GetOffset(const std::string& name) const;

        /**
         * Salva resultados em arquivo JSON
         */
        bool SaveToFile(const char* filename) const;

        /**
         * Carrega resultados de arquivo JSON
         */
        bool LoadFromFile(const char* filename);

        /**
         * Verifica se todos os patterns foram encontrados
         */
        bool IsComplete() const;

        /**
         * Limpa cache e patterns
         */
        void Clear();

        /**
         * Obtém estatísticas de scanning
         */
        void PrintStats() const;
    };

    /**
     * Patterns conhecidos para offsets do GameCoordinator
     */
    namespace GCPatterns {
        // CGCClient::GetGCClientSystem()
        // Pattern: mov rax, [rcx+offset]; test rax, rax
        constexpr const char* GET_GC_CLIENT_SYSTEM = "48 8B 81 ?? ?? ?? ?? 48 85 C0";

        // CGCClientSystem::GetSharedObjectCache()
        // Pattern: mov rax, [rcx+offset]; ret
        constexpr const char* GET_SHARED_OBJECT_CACHE = "48 8B 81 ?? ?? ?? ?? C3";

        // CGCClientSharedObjectTypeCache::GetObjects()
        // Pattern: lea rax, [rcx+offset]; ret
        constexpr const char* GET_OBJECTS_VECTOR = "48 8D 81 ?? ?? ?? ?? C3";

        // C_EconItemView::GetItemDefinition()
        constexpr const char* GET_ITEM_DEFINITION = "48 8B 81 ?? ?? ?? ?? 48 85 C0 74 ?? 48 8B 00";

        // CAttributeList::GetAttributeByDefIndex()
        constexpr const char* GET_ATTRIBUTE_BY_INDEX = "48 8B 41 ?? 8B 54 ?? ?? 3B 54 ?? ?? 75 ??";
    }

    /**
     * Inicializa patterns padrão para CS2
     */
    void InitializeCS2Patterns(PatternCache& cache);

} // namespace PatternScanner
