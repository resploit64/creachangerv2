#pragma once
#include <cstdint>
#include <unordered_map>
#include <string>
#include "fnv1a.h"
#include "schema.h"  // Usa as estruturas já definidas

// Constantes de offset para o SchemaSystem
constexpr std::size_t CS_OFFSETOF_CSchemaSystem_m_pScopeArray = 0x190;
constexpr std::size_t CS_OFFSETOF_CSchemaSystem_m_uScopeSize = 0x198;

// Macros para definir campos usando o Schema System
#define SCHEMA(TYPE, NAME, FIELD) \
[[nodiscard]] __forceinline TYPE NAME() noexcept { \
    static std::uintptr_t uOffset = SchemaSystem::GetOffset(#FIELD); \
    if (uOffset == 0) return TYPE{}; \
    return *reinterpret_cast<TYPE*>(reinterpret_cast<std::uintptr_t>(this) + uOffset); \
}

#define SCHEMA_PTR(TYPE, NAME, FIELD) \
[[nodiscard]] __forceinline TYPE* NAME() noexcept { \
    static std::uintptr_t uOffset = SchemaSystem::GetOffset(#FIELD); \
    if (uOffset == 0) return nullptr; \
    return reinterpret_cast<TYPE*>(reinterpret_cast<std::uintptr_t>(this) + uOffset); \
}

#define SCHEMA_OFFSET(TYPE, NAME, FIELD, ADDITIONAL) \
[[nodiscard]] __forceinline TYPE NAME() noexcept { \
    static std::uintptr_t uOffset = SchemaSystem::GetOffset(#FIELD); \
    if (uOffset == 0) return TYPE{}; \
    return *reinterpret_cast<TYPE*>(reinterpret_cast<std::uintptr_t>(this) + uOffset + ADDITIONAL); \
}

#define OFFSET(TYPE, NAME, OFFSET) \
[[nodiscard]] __forceinline TYPE NAME() noexcept { \
    static std::uintptr_t uOffset = OFFSET; \
    return *reinterpret_cast<TYPE*>(reinterpret_cast<std::uintptr_t>(this) + uOffset); \
}

// Namespace do Schema System
namespace SchemaSystem
{
    // Inicializa o sistema de schema
    bool Setup(const wchar_t* wszModuleName = L"schemasystem.dll");
    
    // Obtém offset de um campo pelo nome (formato: "ClassName->FieldName")
    std::uint32_t GetOffset(const char* szFieldName);
    
    // Obtém offset de um campo específico de uma classe
    std::uint32_t GetOffset(const char* szClassName, const char* szFieldName);
    
    // Limpa o cache de offsets
    void Clear();
    
    // Mapa de offsets cacheados (hash -> offset)
    inline std::unordered_map<std::uint32_t, std::uint32_t> m_mapSchemaOffsets;
    
    // Status de inicialização
    inline bool m_bInitialized = false;
}
