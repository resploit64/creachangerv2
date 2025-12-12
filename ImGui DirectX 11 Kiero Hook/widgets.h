#pragma once
#include <map>
#include <string>
ImU32 GetColorWithAlpha(ImU32 color);
ImU32 GetRarityColor(int rarity);
// Структура для анимации вкладок
struct TabAnimation {
    float hover_alpha = 0.0f;
    float active_alpha = 0.0f;
    float press_scale = 1.0f;
};

namespace W{
	void DrawLogo();
	bool AnimatedTabButton(const char* label, int tab_index, bool is_active, ImVec2 size);
	
	// Универсальная анимированная кнопка
	// label - текст кнопки
	// button_index - уникальный индекс кнопки для анимации
	// size - размер кнопки
	// is_tab_mode - режим работы: true = переключатель (tab), false = обычная кнопка
	// tab_state - указатель на состояние (только для tab режима)
	bool AnimatedButton(const char* label, int button_index, ImVec2 size, bool is_tab_mode = false, bool* tab_state = nullptr);

	// Окно управления скинами из JSON
	void RenderSkinManagerWindow();
	
	// Функции для работы с локализацией и изображениями
	std::string GetLocalizedWeaponName(const std::string& weaponTag);
	std::string GetLocalizedSkinName(const std::string& skinTag);
	std::string GetSkinImagePath(const std::string& weaponTag, const std::string& skinToken);
	std::string GetRarityName(int rarity);
	
	// Функция для отрисовки плиток скинов
	bool DrawSkinTile(const char* text, const ImVec2& size, int rarity, bool isSelected = false);

}