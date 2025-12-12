#include "SchemaSystem.h"
#include "memory.h"
#include "logger.h"
#include <cstring>
#include <vector>
#include <windows.h>
#include <psapi.h>

namespace SchemaSystem
{
    // Helper para ler memória segura
    template<typename T>
    T ReadMemory(std::uintptr_t address)
    {
        if (address == 0 || IsBadReadPtr(reinterpret_cast<void*>(address), sizeof(T)))
            return T{};
        return *reinterpret_cast<T*>(address);
    }

    // Helper para ler buffer de memória
    bool ReadMemoryRaw(void* address, void* buffer, std::size_t size)
    {
        if (address == nullptr || buffer == nullptr || size == 0)
            return false;
        if (IsBadReadPtr(address, size))
            return false;
        
        std::memcpy(buffer, address, size);
        return true;
    }

    bool Setup(const wchar_t* wszModuleName)
    {
        LOG_INFO("[SchemaSystem] Iniciando setup do Schema System...");
        
        // Obtém o endereço do módulo schemasystem.dll
        std::uintptr_t uSchemaModule = reinterpret_cast<std::uintptr_t>(M::GetModuleBaseHandle(wszModuleName));
        if (uSchemaModule == 0)
        {
            LOG_ERROR("[SchemaSystem] Falha ao encontrar módulo schemasystem.dll");
            return false;
        }
        
        LOG_DEBUG("[SchemaSystem] Módulo schemasystem.dll encontrado em: 0x%llX", (unsigned long long)uSchemaModule);
        
        // Pattern para encontrar a interface do SchemaSystem
        // Padrão: 48 8D 05 ? ? ? ? C3 CC CC CC CC CC CC CC CC 48 89 5C 24
        const char* szPattern = "48 8D 05 ? ? ? ? C3 CC CC CC CC CC CC CC CC 48 89 5C 24";
        std::uint8_t* pSchemaInterface = M::FindPattern(wszModuleName, szPattern);
        
        if (pSchemaInterface == nullptr)
        {
            LOG_ERROR("[SchemaSystem] Falha ao encontrar pattern da interface SchemaSystem");
            return false;
        }
        
        LOG_DEBUG("[SchemaSystem] Pattern encontrado em: 0x%llX", (unsigned long long)reinterpret_cast<std::uintptr_t>(pSchemaInterface));
        
        // Resolve o endereço RIP-relative
        std::uintptr_t uSchemaInterfaceAddress = reinterpret_cast<std::uintptr_t>(
            M::ResolveRelativeAddress(pSchemaInterface, 0x3, 0x7)
        );
        
        if (uSchemaInterfaceAddress == 0)
        {
            LOG_ERROR("[SchemaSystem] Falha ao resolver endereço da interface SchemaSystem");
            return false;
        }
        
        LOG_DEBUG("[SchemaSystem] Interface SchemaSystem em: 0x%llX", (unsigned long long)uSchemaInterfaceAddress);
        
        // Lê o ponteiro do scope array
        std::uintptr_t uSchemaSystemScopeArrayPtr = 0U;
        if (!ReadMemoryRaw(
            reinterpret_cast<void*>(uSchemaInterfaceAddress + CS_OFFSETOF_CSchemaSystem_m_pScopeArray),
            &uSchemaSystemScopeArrayPtr,
            sizeof(std::uintptr_t)))
        {
            LOG_ERROR("[SchemaSystem] Falha ao ler ponteiro do scope array");
            return false;
        }
        
        if (uSchemaSystemScopeArrayPtr == 0)
        {
            LOG_ERROR("[SchemaSystem] Ponteiro do scope array é NULL");
            return false;
        }
        
        LOG_DEBUG("[SchemaSystem] Scope array pointer: 0x%llX", (unsigned long long)uSchemaSystemScopeArrayPtr);
        
        // Lê o tamanho do scope
        int nScopeSize = ReadMemory<int>(uSchemaInterfaceAddress + CS_OFFSETOF_CSchemaSystem_m_uScopeSize);
        if (nScopeSize <= 0 || nScopeSize > 1000)
        {
            LOG_ERROR("[SchemaSystem] Tamanho de scope inválido: %d", nScopeSize);
            return false;
        }
        
        LOG_DEBUG("[SchemaSystem] Scope size: %d", nScopeSize);
        
        // Aloca array de ponteiros para scopes
        std::vector<void*> vecScopeArray(nScopeSize);
        if (!ReadMemoryRaw(
            reinterpret_cast<void*>(uSchemaSystemScopeArrayPtr),
            vecScopeArray.data(),
            nScopeSize * sizeof(void*)))
        {
            LOG_ERROR("[SchemaSystem] Falha ao ler scope array");
            return false;
        }
        
        int nOffsetsFound = 0;
        bool bFoundClientDll = false;
        
        // Itera por todos os scopes
        for (int i = 0; i < nScopeSize; ++i)
        {
            if (vecScopeArray[i] == nullptr)
                continue;
            
            CSchemaSystemTypeScope schemaScope{};
            if (!ReadMemoryRaw(vecScopeArray[i], &schemaScope, sizeof(CSchemaSystemTypeScope)))
                continue;
            
            if (schemaScope.declaredClasses == nullptr)
                continue;
            
            // Verifica se é o scope do client.dll
            if (std::strcmp(schemaScope.name, "client.dll") == 0)
            {
                bFoundClientDll = true;
                LOG_INFO("[SchemaSystem] Encontrado scope 'client.dll' com %d classes", schemaScope.numDeclaredClasses);
                
                // Aloca array de entradas de classes declaradas
                std::vector<CSchemaDeclaredClassEntry> vecDeclaredClassEntries(schemaScope.numDeclaredClasses + 1);
                if (!ReadMemoryRaw(
                    schemaScope.declaredClasses,
                    vecDeclaredClassEntries.data(),
                    (schemaScope.numDeclaredClasses + 1) * sizeof(CSchemaDeclaredClassEntry)))
                {
                    LOG_WARNING("[SchemaSystem] Falha ao ler declared class entries");
                    continue;
                }
                
                // Itera por todas as classes declaradas
                for (std::uint16_t j = 0; j < schemaScope.numDeclaredClasses; ++j)
                {
                    if (vecDeclaredClassEntries[j].declaredClass == nullptr)
                        continue;
                    
                    CSchemaDeclaredClass declaredClass{};
                    if (!ReadMemoryRaw(vecDeclaredClassEntries[j].declaredClass, &declaredClass, sizeof(CSchemaDeclaredClass)))
                        continue;
                    
                    if (declaredClass.mClass == nullptr)
                        continue;
                    
                    SchemaClassInfoData_t schemaClass{};
                    if (!ReadMemoryRaw(declaredClass.mClass, &schemaClass, sizeof(SchemaClassInfoData_t)))
                        continue;
                    
                    char szClassName[256]{};
                    if (!ReadMemoryRaw(const_cast<char*>(declaredClass.name), szClassName, sizeof(szClassName)))
                        continue;
                    
                    // Lê os campos da classe
                    std::uintptr_t uClassFieldsPtr = reinterpret_cast<std::uintptr_t>(schemaClass.fields);
                    if (uClassFieldsPtr == 0)
                        continue;
                    
                    for (std::uint16_t k = 0; k < schemaClass.numFields; ++k)
                    {
                        SchemaClassFieldData_t schemaField = ReadMemory<SchemaClassFieldData_t>(uClassFieldsPtr + (sizeof(SchemaClassFieldData_t) * k));
                        if (schemaField.type == nullptr || schemaField.name == nullptr)
                            continue;
                        
                        char szFieldName[256] = {0};
                        if (!ReadMemoryRaw(const_cast<char*>(schemaField.name), szFieldName, sizeof(szFieldName)))
                            continue;
                        
                        // Cria chave no formato "ClassName->FieldName"
                        char strSchemaField[512];
                        snprintf(strSchemaField, sizeof(strSchemaField), "%s->%s", szClassName, szFieldName);
                        
                        // Calcula hash FNV1A e armazena o offset
                        std::uint32_t hash = fnv1a::hash_32(strSchemaField);
                        m_mapSchemaOffsets[hash] = schemaField.offset;
                        
                        nOffsetsFound++;
                    }
                }
            }
        }
        
        m_bInitialized = (nOffsetsFound > 0);
        
        if (m_bInitialized)
        {
            LOG_INFO("[SchemaSystem] Setup concluído com sucesso! %d offsets carregados", nOffsetsFound);
        }
        else
        {
            LOG_ERROR("[SchemaSystem] Setup falhou - nenhum offset encontrado");
            if (!bFoundClientDll)
            {
                LOG_ERROR("[SchemaSystem] Scope 'client.dll' não encontrado!");
            }
        }
        
        return m_bInitialized;
    }
    
    std::uint32_t GetOffset(const char* szFieldName)
    {
        if (!m_bInitialized)
        {
            LOG_WARNING("[SchemaSystem] GetOffset chamado antes da inicialização!");
            return 0;
        }
        
        std::uint32_t hash = fnv1a::hash_32(szFieldName);
        auto it = m_mapSchemaOffsets.find(hash);
        
        if (it != m_mapSchemaOffsets.end())
        {
            return it->second;
        }
        
        LOG_DEBUG("[SchemaSystem] Offset não encontrado para: %s", szFieldName);
        return 0;
    }
    
    std::uint32_t GetOffset(const char* szClassName, const char* szFieldName)
    {
        char strFullName[512];
        snprintf(strFullName, sizeof(strFullName), "%s->%s", szClassName, szFieldName);
        return GetOffset(strFullName);
    }
    
    void Clear()
    {
        m_mapSchemaOffsets.clear();
        m_bInitialized = false;
        LOG_INFO("[SchemaSystem] Cache de offsets limpo");
    }
}
