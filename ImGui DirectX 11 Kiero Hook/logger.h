/**
 * logger.h - Sistema de Logging Robusto para creatinawtf.dll
 * 
 * Fornece logging thread-safe com timestamps para console do jogo e arquivo.
 * Níveis: DEBUG, INFO, WARNING, ERROR
 */

#pragma once
#include <string>
#include <fstream>
#include <ctime>
#include <chrono>
#include <iomanip>
#include <sstream>
#include <windows.h>

namespace CreatinaLogger {

    // Níveis de log
    enum class LogLevel {
        DEBUG,
        INFO,
        WARNING,
        ERR  // Renomeado de ERROR para evitar conflito com macro do Windows.h
    };

    class Logger {
    private:
        static std::ofstream logFile;
        static CRITICAL_SECTION logLock;  // Usando CRITICAL_SECTION do Windows ao invés de mutex
        static bool lockInitialized;
        static bool initialized;
        static LogLevel currentLevel;
        static const char* logFilePath;

        // Função interna para formatar timestamp
        static std::string GetTimestamp() {
            auto now = std::chrono::system_clock::now();
            auto in_time_t = std::chrono::system_clock::to_time_t(now);
            auto ms = std::chrono::duration_cast<std::chrono::milliseconds>(
                now.time_since_epoch()) % 1000;

            std::tm bt{};
            localtime_s(&bt, &in_time_t);

            std::ostringstream oss;
            oss << std::put_time(&bt, "%Y-%m-%d %H:%M:%S");
            oss << '.' << std::setfill('0') << std::setw(3) << ms.count();
            return oss.str();
        }

        // Função interna para converter LogLevel para string
        static const char* LogLevelToString(LogLevel level) {
            switch (level) {
            case LogLevel::DEBUG:   return "DEBUG";
            case LogLevel::INFO:    return "INFO ";
            case LogLevel::WARNING: return "WARN ";
            case LogLevel::ERR:     return "ERROR";
            default:                return "UNKNW";
            }
        }

        // Função interna para log formatado
        static void WriteLog(LogLevel level, const char* message);

    public:
        // Inicializa o sistema de logging
        static void Initialize(LogLevel minLevel = LogLevel::DEBUG);

        // Finaliza o sistema de logging
        static void Shutdown();

        // Funções de logging por nível
        static void Debug(const char* format, ...);
        static void Info(const char* format, ...);
        static void Warning(const char* format, ...);
        static void Error(const char* format, ...);

        // Log de separator para organização
        static void Separator();

        // Flush forçado do arquivo
        static void Flush();

        // Verifica se está inicializado
        static bool IsInitialized() { return initialized; }
    };

    // Macros para facilitar o uso
    #define LOG_DEBUG(fmt, ...)    CreatinaLogger::Logger::Debug(fmt, ##__VA_ARGS__)
    #define LOG_INFO(fmt, ...)     CreatinaLogger::Logger::Info(fmt, ##__VA_ARGS__)
    #define LOG_WARNING(fmt, ...)  CreatinaLogger::Logger::Warning(fmt, ##__VA_ARGS__)
    #define LOG_ERROR(fmt, ...)    CreatinaLogger::Logger::Error(fmt, ##__VA_ARGS__)
    #define LOG_SEPARATOR()        CreatinaLogger::Logger::Separator()
}
