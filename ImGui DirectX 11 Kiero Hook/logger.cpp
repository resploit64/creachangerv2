/**
 * logger.cpp - Implementação do sistema de logging
 */

#include "logger.h"
#include <cstdarg>
#include <cstdio>

namespace CreatinaLogger {

    // Inicialização de membros estáticos
    std::ofstream Logger::logFile;
    CRITICAL_SECTION Logger::logLock;
    bool Logger::lockInitialized = false;
    bool Logger::initialized = false;
    LogLevel Logger::currentLevel = LogLevel::DEBUG;
    const char* Logger::logFilePath = "C:\\creatinawtf_log.txt";

    void Logger::Initialize(LogLevel minLevel) {
        if (!lockInitialized) {
            InitializeCriticalSection(&logLock);
            lockInitialized = true;
        }
        
        EnterCriticalSection(&logLock);

        if (initialized) {
            LeaveCriticalSection(&logLock);
            return; // Já inicializado
        }

        currentLevel = minLevel;

        // Tenta abrir o arquivo de log
        logFile.open(logFilePath, std::ios::out | std::ios::app);
        
        if (!logFile.is_open()) {
            // Fallback: tenta em temp
            const char* tempPath = "C:\\temp\\creatinawtf_log.txt";
            logFile.open(tempPath, std::ios::out | std::ios::app);
            
            if (!logFile.is_open()) {
                // Se ainda falhar, continua sem arquivo (apenas console)
                fprintf(stderr, "[LOGGER] WARNING: Could not open log file. Logging to console only.\n");
            } else {
                logFilePath = tempPath;
            }
        }

        initialized = true;
        
        LeaveCriticalSection(&logLock);

        // Log de inicialização
        Separator();
        Info("=== creatinawtf.dll Logger Initialized ===");
        Info("Log file: %s", logFilePath);
        Info("Minimum log level: %s", LogLevelToString(minLevel));
        Separator();
    }

    void Logger::Shutdown() {
        if (!initialized || !lockInitialized) return;
        
        EnterCriticalSection(&logLock);

        Separator();
        Info("=== creatinawtf.dll Logger Shutdown ===");
        Separator();

        if (logFile.is_open()) {
            logFile.flush();
            logFile.close();
        }

        initialized = false;
        
        LeaveCriticalSection(&logLock);
        
        DeleteCriticalSection(&logLock);
        lockInitialized = false;
    }

    void Logger::WriteLog(LogLevel level, const char* message) {
        if (!initialized) {
            // Fallback para printf se não inicializado
            printf("%s\n", message);
            return;
        }

        // Verifica se o nível do log é suficiente
        if (level < currentLevel) {
            return;
        }

        if (!lockInitialized) return;
        EnterCriticalSection(&logLock);

        std::string timestamp = GetTimestamp();
        const char* levelStr = LogLevelToString(level);

        char formattedMessage[4096];
        snprintf(formattedMessage, sizeof(formattedMessage), 
                 "[%s] [%s] %s", timestamp.c_str(), levelStr, message);

        // Log para console do jogo (via printf)
        printf("%s\n", formattedMessage);

        // Log para arquivo
        if (logFile.is_open()) {
            logFile << formattedMessage << std::endl;
            logFile.flush(); // Flush imediato para capturar logs de crash
        }
        
        LeaveCriticalSection(&logLock);
    }

    void Logger::Debug(const char* format, ...) {
        char buffer[4096];
        va_list args;
        va_start(args, format);
        vsnprintf(buffer, sizeof(buffer), format, args);
        va_end(args);

        WriteLog(LogLevel::DEBUG, buffer);
    }

    void Logger::Info(const char* format, ...) {
        char buffer[4096];
        va_list args;
        va_start(args, format);
        vsnprintf(buffer, sizeof(buffer), format, args);
        va_end(args);

        WriteLog(LogLevel::INFO, buffer);
    }

    void Logger::Warning(const char* format, ...) {
        char buffer[4096];
        va_list args;
        va_start(args, format);
        vsnprintf(buffer, sizeof(buffer), format, args);
        va_end(args);

        WriteLog(LogLevel::WARNING, buffer);
    }

    void Logger::Error(const char* format, ...) {
        char buffer[4096];
        va_list args;
        va_start(args, format);
        vsnprintf(buffer, sizeof(buffer), format, args);
        va_end(args);

        WriteLog(LogLevel::ERR, buffer);
    }

    void Logger::Separator() {
        if (!initialized || !lockInitialized) return;

        EnterCriticalSection(&logLock);

        const char* separator = "=================================================================================";
        
        printf("%s\n", separator);
        
        if (logFile.is_open()) {
            logFile << separator << std::endl;
            logFile.flush();
        }
        
        LeaveCriticalSection(&logLock);
    }

    void Logger::Flush() {
        if (!initialized || !lockInitialized) return;

        EnterCriticalSection(&logLock);

        if (logFile.is_open()) {
            logFile.flush();
        }
        
        LeaveCriticalSection(&logLock);
    }
}
