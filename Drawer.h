#pragma once

#include <afxwin.h>
#include <vector>
#include <algorithm>
#include <gdiplus.h>  // Добавляем GDI+
#define pi 3.1415926535897932384626433832795

#undef min
#undef max

using namespace std;

class Drawer
{
private:
	CRect frame;
	CWnd* wnd;
	CDC* dc;
	CDC memDC;
	CBitmap bmp;
	bool init;
	ULONG_PTR m_gdiplusToken;

	// Добавляем переменные для диапазона
	double m_freqMin;
	double m_freqMax;
	double m_timeMin;
	double m_timeMax;
	bool m_customRange;

public:
	Drawer() : init(false), m_gdiplusToken(0),
		m_freqMin(0.0), m_freqMax(0.0),
		m_timeMin(0.0), m_timeMax(0.0),
		m_customRange(false)
	{
		Gdiplus::GdiplusStartupInput gdiplusStartupInput;
		Gdiplus::GdiplusStartup(&m_gdiplusToken, &gdiplusStartupInput, NULL);
	}

	~Drawer()
	{
		if (m_gdiplusToken != 0)
			Gdiplus::GdiplusShutdown(m_gdiplusToken);
	}


	// Установить диапазон частот и времени для отображения
	void SetViewRange(double freq_min, double freq_max, double time_min, double time_max)
	{
		m_freqMin = freq_min;
		m_freqMax = freq_max;
		m_timeMin = time_min;
		m_timeMax = time_max;
		m_customRange = true;
	}

	// Сбросить к автоматическому определению диапазона
	void ResetViewRange()
	{
		m_customRange = false;
	}

	// Получить текущий диапазон
	void GetViewRange(double& freq_min, double& freq_max,
		double& time_min, double& time_max)
	{
		freq_min = m_freqMin;
		freq_max = m_freqMax;
		time_min = m_timeMin;
		time_max = m_timeMax;
	}

	// Проверить, используется ли пользовательский диапазон
	bool IsCustomRange() const { return m_customRange; }

	void Create(HWND hWnd)
	{
		wnd = CWnd::FromHandle(hWnd);
		wnd->GetClientRect(frame);
		dc = wnd->GetDC();

		memDC.CreateCompatibleDC(dc);
		bmp.CreateCompatibleBitmap(dc, frame.Width(), frame.Height());
		memDC.SelectObject(&bmp);
		init = true;
	}

	void DrawAmbiguityFunctionContour(
		vector<vector<double>>& data,
		vector<double>& freq_values,
		vector<double>& time_values,  // время в секундах
		CString title = L"Взаимная функция неопределенности",
		CString x_label = L"Δf, Гц",
		CString y_label = L"Δt, мс",
		bool show_max_point = true,
		double calculated_max_freq = 0.0,
		double calculated_max_time = 0.0,  // время в секундах
		int contour_levels = 8,           // количество уровней изолиний
		bool invert_colors = true)         // инвертировать цвета
	{
		if (!init || data.empty() || freq_values.empty() || time_values.empty())
			return;

		CDC* dc = wnd->GetDC();
		HDC hdc = *dc;
		Gdiplus::Graphics gr(hdc);

		CRect rect;
		wnd->GetClientRect(&rect);

		Gdiplus::Bitmap myBitmap(rect.Width(), rect.Height());
		Gdiplus::Graphics* grr = Gdiplus::Graphics::FromImage(&myBitmap);
		grr->SetSmoothingMode(Gdiplus::SmoothingModeHighQuality);
		grr->Clear(Gdiplus::Color::WhiteSmoke);

		// Находим min/max значений функции
		double min_val = data[0][0];
		double max_val = data[0][0];
		for (const auto& row : data) {
			for (double val : row) {
				if (val < min_val) min_val = val;
				if (val > max_val) max_val = val;
			}
		}

		// Определяем диапазоны для отображения
		double display_freq_min, display_freq_max;
		double display_time_min_sec, display_time_max_sec;

		if (m_customRange) {
			display_freq_min = m_freqMin;
			display_freq_max = m_freqMax;
			display_time_min_sec = m_timeMin / 1000.0;
			display_time_max_sec = m_timeMax / 1000.0;
		}
		else {
			display_freq_min = *std::min_element(freq_values.begin(), freq_values.end());
			display_freq_max = *std::max_element(freq_values.begin(), freq_values.end());
			display_time_min_sec = *std::min_element(time_values.begin(), time_values.end());
			display_time_max_sec = *std::max_element(time_values.begin(), time_values.end());
		}

		// Фильтруем данные для заданного диапазона
		vector<vector<double>> filtered_data;
		vector<double> filtered_freq;
		vector<double> filtered_time_sec;

		FilterDataForRange(data, freq_values, time_values,
			display_freq_min, display_freq_max,
			display_time_min_sec, display_time_max_sec,
			filtered_data, filtered_freq, filtered_time_sec);

		bool use_filtered = !filtered_data.empty();
		vector<vector<double>>& display_data = use_filtered ? filtered_data : data;
		vector<double>& display_freq = use_filtered ? filtered_freq : freq_values;
		vector<double>& display_time_sec = use_filtered ? filtered_time_sec : time_values;

		// Обновляем min/max для отображаемых данных
		if (!display_data.empty()) {
			min_val = display_data[0][0];
			max_val = display_data[0][0];
			for (const auto& row : display_data) {
				for (double val : row) {
					if (val < min_val) min_val = val;
					if (val > max_val) max_val = val;
				}
			}
		}

		// Настройка области отрисовки
		int padding_left = 60;
		int padding_right = 80;
		int padding_top = 40;
		int padding_bottom = 40;

		int plot_width = rect.Width() - padding_left - padding_right;
		int plot_height = rect.Height() - padding_top - padding_bottom;
		int plot_x = padding_left;
		int plot_y = padding_top;

		int rows = display_data.size();
		int cols = (rows > 0) ? display_data[0].size() : 0;

		if (rows == 0 || cols == 0) {
			delete grr;
			return;
		}

		// Подготовка данных для изолиний
		vector<vector<double>> log_data;
		double log_min = log(min_val > 0 ? min_val : 0.001);
		double log_max = log(max_val > 0 ? max_val : 1.0);

		// Создаем логарифмические данные (как в Python: 10*log10)
		for (int i = 0; i < rows; ++i) {
			vector<double> log_row;
			for (int j = 0; j < cols; ++j) {
				double val = display_data[i][j];
				double log_val = val > 0 ? 10.0 * log10(val) : log_min;
				log_row.push_back(log_val);
			}
			log_data.push_back(log_row);
		}

		// Обновляем min/max для логарифмических данных
		double log_data_min = log_data[0][0];
		double log_data_max = log_data[0][0];
		for (const auto& row : log_data) {
			for (double val : row) {
				if (val < log_data_min) log_data_min = val;
				if (val > log_data_max) log_data_max = val;
			}
		}

		// Преобразования координат
		double freq_range = display_freq_max - display_freq_min;
		double time_range = display_time_max_sec - display_time_min_sec;
		double freq_to_pixel_x = (freq_range != 0) ? plot_width / freq_range : 1.0;
		double time_to_pixel_y = (time_range != 0) ? plot_height / time_range : 1.0;

		// Рисуем тепловую карту (с инвертированными или обычными цветами)
		for (int i = 0; i < rows; ++i) {
			double time_val_sec = display_time_sec[i];
			double normalized_time = time_val_sec - display_time_min_sec;
			double y_display = plot_y + normalized_time * time_to_pixel_y;

			for (int j = 0; j < cols; ++j) {
				double freq_val = display_freq[j];
				double val = log_data[i][j];
				double normalized_val = (val - log_data_min) / (log_data_max - log_data_min);

				// Инвертирование цвета если нужно
				if (invert_colors) {
					normalized_val = 1.0 - normalized_val;
				}

				int r, g, b;
				GetInfernoColor(normalized_val, r, g, b);
				Gdiplus::SolidBrush br(Gdiplus::Color(255, r, g, b));

				double normalized_freq = freq_val - display_freq_min;
				double x = plot_x + normalized_freq * freq_to_pixel_x;

				double cell_width = (j < cols - 1) ?
					(display_freq[j + 1] - freq_val) * freq_to_pixel_x :
					freq_to_pixel_x * (freq_range / cols);

				double cell_height = (i < rows - 1) ?
					(display_time_sec[i + 1] - time_val_sec) * time_to_pixel_y :
					time_to_pixel_y * (time_range / rows);

				cell_width = std::max(1.0, std::abs(cell_width));
				cell_height = std::max(1.0, std::abs(cell_height));

				grr->FillRectangle(&br,
					Gdiplus::REAL(x),
					Gdiplus::REAL(y_display),
					Gdiplus::REAL(cell_width),
					Gdiplus::REAL(cell_height));
			}
		}

		// Рисуем изолинии с помощью упрощенного алгоритма марширующих квадратов
		if (contour_levels > 0) {
			// Создаем уровни для изолиний
			vector<double> contour_level_values;
			for (int i = 0; i <= contour_levels; ++i) {
				double level = log_data_min + i * (log_data_max - log_data_min) / contour_levels;
				contour_level_values.push_back(level);
			}

			// Разные стили линий для разных уровней
			vector<Gdiplus::Color> contour_colors = {
				Gdiplus::Color::Black,
				Gdiplus::Color(100, 100, 100),    // темно-серый
				Gdiplus::Color(150, 150, 150),    // серый
				Gdiplus::Color::Black,
				Gdiplus::Color(100, 100, 100),
				Gdiplus::Color(150, 150, 150),
				Gdiplus::Color::Black,
				Gdiplus::Color(100, 100, 100)
			};

			// Для каждого уровня рисуем изолинии
			for (int level_idx = 0; level_idx < contour_level_values.size(); ++level_idx) {
				double level = contour_level_values[level_idx];

				// Пропускаем крайние уровни, если нужно
				if (level_idx == 0 || level_idx == contour_level_values.size() - 1) {
					continue;
				}

				Gdiplus::Color line_color = contour_colors[level_idx % contour_colors.size()];
				Gdiplus::Pen contour_pen(line_color, 1.0f);

				// Стиль линии: чередование сплошных и пунктирных линий
				if (level_idx % 2 == 0) {
					contour_pen.SetDashStyle(Gdiplus::DashStyleDash);
				}

				// Упрощенный алгоритм: рисуем линии между точками, где значение пересекает уровень
				for (int i = 0; i < rows - 1; ++i) {
					double y1 = plot_y + (display_time_sec[i] - display_time_min_sec) * time_to_pixel_y;
					double y2 = plot_y + (display_time_sec[i + 1] - display_time_min_sec) * time_to_pixel_y;
					double cell_height = y2 - y1;

					for (int j = 0; j < cols - 1; ++j) {
						double x1 = plot_x + (display_freq[j] - display_freq_min) * freq_to_pixel_x;
						double x2 = plot_x + (display_freq[j + 1] - display_freq_min) * freq_to_pixel_x;
						double cell_width = x2 - x1;

						// Значения в четырех углах ячейки
						double val00 = log_data[i][j];
						double val01 = log_data[i][j + 1];
						double val10 = log_data[i + 1][j];
						double val11 = log_data[i + 1][j + 1];

						// Проверяем, пересекает ли уровень эту ячейку
						bool crosses_level = false;

						// Массив точек для линии
						vector<pair<double, double>> line_points;

						// Проверяем каждую сторону квадрата

						// Верхняя сторона (между val00 и val01)
						if ((val00 < level && val01 >= level) || (val00 >= level && val01 < level)) {
							double t = (level - val00) / (val01 - val00);
							double x = x1 + t * cell_width;
							line_points.push_back({ x, y1 });
							crosses_level = true;
						}

						// Нижняя сторона (между val10 и val11)
						if ((val10 < level && val11 >= level) || (val10 >= level && val11 < level)) {
							double t = (level - val10) / (val11 - val10);
							double x = x1 + t * cell_width;
							line_points.push_back({ x, y2 });
							crosses_level = true;
						}

						// Левая сторона (между val00 и val10)
						if ((val00 < level && val10 >= level) || (val00 >= level && val10 < level)) {
							double t = (level - val00) / (val10 - val00);
							double y = y1 + t * cell_height;
							line_points.push_back({ x1, y });
							crosses_level = true;
						}

						// Правая сторона (между val01 и val11)
						if ((val01 < level && val11 >= level) || (val01 >= level && val11 < level)) {
							double t = (level - val01) / (val11 - val01);
							double y = y1 + t * cell_height;
							line_points.push_back({ x2, y });
							crosses_level = true;
						}

						// Если уровень пересекает ячейку, рисуем линию
						if (crosses_level && line_points.size() >= 2) {
							// Если 2 точки - рисуем прямую линию
							if (line_points.size() == 2) {
								grr->DrawLine(&contour_pen,
									Gdiplus::REAL(line_points[0].first),
									Gdiplus::REAL(line_points[0].second),
									Gdiplus::REAL(line_points[1].first),
									Gdiplus::REAL(line_points[1].second));
							}
							// Если больше 2 точек (редкий случай), рисуем полилинию
							else if (line_points.size() > 2) {
								Gdiplus::PointF* points = new Gdiplus::PointF[line_points.size()];
								for (size_t idx = 0; idx < line_points.size(); ++idx) {
									points[idx] = Gdiplus::PointF(
										Gdiplus::REAL(line_points[idx].first),
										Gdiplus::REAL(line_points[idx].second));
								}
								grr->DrawLines(&contour_pen, points, (INT)line_points.size());
								delete[] points;
							}
						}
					}
				}
			}
		}

		// Отображение точки максимума
		if (show_max_point && (calculated_max_time != 0.0 || calculated_max_freq != 0.0)) {
			double target_freq = calculated_max_freq;
			double target_time_sec = calculated_max_time;

			bool freq_in_range = (target_freq >= display_freq_min && target_freq <= display_freq_max);
			bool time_in_range = (target_time_sec >= display_time_min_sec && target_time_sec <= display_time_max_sec);

			if (freq_in_range && time_in_range) {
				double normalized_freq = target_freq - display_freq_min;
				double normalized_time = target_time_sec - display_time_min_sec;

				double center_x = plot_x + normalized_freq * freq_to_pixel_x;
				double center_y = plot_y + normalized_time * time_to_pixel_y;

				if (center_x >= plot_x && center_x <= plot_x + plot_width &&
					center_y >= plot_y && center_y <= plot_y + plot_height) {
					Gdiplus::Pen white_pen(Gdiplus::Color::White, 2);

					// Белый крест
					grr->DrawLine(&white_pen,
						Gdiplus::PointF((Gdiplus::REAL)(center_x - 8), (Gdiplus::REAL)center_y),
						Gdiplus::PointF((Gdiplus::REAL)(center_x + 8), (Gdiplus::REAL)center_y));
					grr->DrawLine(&white_pen,
						Gdiplus::PointF((Gdiplus::REAL)center_x, (Gdiplus::REAL)(center_y - 8)),
						Gdiplus::PointF((Gdiplus::REAL)center_x, (Gdiplus::REAL)(center_y + 8)));

					// Подпись с максимальным значением
					double max_val_in_data = 0;
					int max_i = 0, max_j = 0;
					for (int i = 0; i < rows; ++i) {
						for (int j = 0; j < cols; ++j) {
							if (display_data[i][j] > max_val_in_data) {
								max_val_in_data = display_data[i][j];
								max_i = i;
								max_j = j;
							}
						}
					}

					Gdiplus::Font font(L"Arial", 10, Gdiplus::FontStyleBold);
					Gdiplus::SolidBrush text_brush(Gdiplus::Color::White);
					CString label;
					label.Format(L"Max: %.2f", max_val_in_data);

					grr->DrawString(label, -1, &font,
						Gdiplus::PointF((Gdiplus::REAL)(center_x + 10),
							(Gdiplus::REAL)(center_y - 10)),
						&text_brush);
				}
			}
		}

		// Рисуем оси
		DrawAxesForAmbiguity(*grr, plot_x, plot_y, plot_width, plot_height,
			display_freq_min, display_freq_max,
			display_time_min_sec * 1000.0, display_time_max_sec * 1000.0);

		DrawColorScale(*grr, max_val, min_val,
			plot_x + plot_width + 10, plot_y, 20, plot_height);

		// Заголовок
		CString full_title = title;
		if (invert_colors) {
			full_title += L" (инвертировано)";
		}

		DrawTitleAndLabels(*grr, rect.Width(), plot_x, plot_y, plot_width, plot_height,
			full_title, x_label, y_label);

		// Копируем на экран
		gr.DrawImage(&myBitmap, 0, 0, rect.Width(), rect.Height());
		delete grr;
	}


	void DrawAmbiguityFunction(vector<vector<double>>& data,
		vector<double>& freq_values,
		vector<double>& time_values,  // время в секундах
		CString title = L"Ambiguity Function",
		CString x_label = L"Δf, Гц",
		CString y_label = L"Δt, мс",
		bool show_max_point = true,
		double calculated_max_freq = 0.0,
		double calculated_max_time = 0.0)  // время в секундах
	{
		if (!init || data.empty() || freq_values.empty() || time_values.empty())
			return;

		CDC* dc = wnd->GetDC();
		HDC hdc = *dc;
		Gdiplus::Graphics gr(hdc);

		CRect rect;
		wnd->GetClientRect(&rect);

		Gdiplus::Bitmap myBitmap(rect.Width(), rect.Height());
		Gdiplus::Graphics* grr = Gdiplus::Graphics::FromImage(&myBitmap);
		grr->SetSmoothingMode(Gdiplus::SmoothingModeHighSpeed);
		grr->Clear(Gdiplus::Color::WhiteSmoke);

		// Находим min/max значений функции
		double min_val = data[0][0];
		double max_val = data[0][0];
		for (const auto& row : data) {
			for (double val : row) {
				if (val < min_val) min_val = val;
				if (val > max_val) max_val = val;
			}
		}

		// Определяем диапазоны для отображения (в правильных единицах)
		double display_freq_min, display_freq_max;
		double display_time_min_sec, display_time_max_sec;  // время в секундах

		if (m_customRange) {
			// Используем заданный пользователем диапазон
			// ПРЕОБРАЗОВАНИЕ: пользователь вводит мс, нам нужны секунды
			display_freq_min = m_freqMin;
			display_freq_max = m_freqMax;
			display_time_min_sec = m_timeMin / 1000.0;      // мс -> сек
			display_time_max_sec = m_timeMax / 1000.0;      // мс -> сек
		}
		else {
			// Автоматически определяем диапазон из данных
			display_freq_min = *std::min_element(freq_values.begin(), freq_values.end());
			display_freq_max = *std::max_element(freq_values.begin(), freq_values.end());

			// ВРЕМЯ: находим мин/макс из time_values (секунды)
			display_time_min_sec = *std::min_element(time_values.begin(), time_values.end());
			display_time_max_sec = *std::max_element(time_values.begin(), time_values.end());
		}

		// Фильтруем данные для заданного диапазона (ТЕПЕРЬ В СЕКУНДАХ)
		vector<vector<double>> filtered_data;
		vector<double> filtered_freq;
		vector<double> filtered_time_sec;  // время в секундах

		FilterDataForRange(data, freq_values, time_values,
			display_freq_min, display_freq_max,
			display_time_min_sec, display_time_max_sec,  // передаем секунды
			filtered_data, filtered_freq, filtered_time_sec);

		// Если данные отфильтрованы, используем их, иначе используем оригинальные
		bool use_filtered = !filtered_data.empty();
		vector<vector<double>>& display_data = use_filtered ? filtered_data : data;
		vector<double>& display_freq = use_filtered ? filtered_freq : freq_values;
		vector<double>& display_time_sec = use_filtered ? filtered_time_sec : time_values;

		// Если пользовательский диапазон, но данные не отфильтровались, 
		// значит весь диапазон внутри пользовательских границ
		if (!use_filtered && m_customRange) {
			display_freq_min = m_freqMin;
			display_freq_max = m_freqMax;
			display_time_min_sec = m_timeMin / 1000.0;
			display_time_max_sec = m_timeMax / 1000.0;
		}
		else if (!use_filtered) {
			display_freq_min = *std::min_element(freq_values.begin(), freq_values.end());
			display_freq_max = *std::max_element(freq_values.begin(), freq_values.end());
			display_time_min_sec = *std::min_element(time_values.begin(), time_values.end());
			display_time_max_sec = *std::max_element(time_values.begin(), time_values.end());
		}

		// Пересчитываем min/max для отображаемых данных
		if (!display_data.empty()) {
			min_val = display_data[0][0];
			max_val = display_data[0][0];
			for (const auto& row : display_data) {
				for (double val : row) {
					if (val < min_val) min_val = val;
					if (val > max_val) max_val = val;
				}
			}
		}

		// Настройка области отрисовки
		int padding_left = 60;
		int padding_right = 80;
		int padding_top = 40;
		int padding_bottom = 40;

		int plot_width = rect.Width() - padding_left - padding_right;
		int plot_height = rect.Height() - padding_top - padding_bottom;
		int plot_x = padding_left;
		int plot_y = padding_top;

		// Размеры отображаемых данных
		int rows = display_data.size();
		int cols = (rows > 0) ? display_data[0].size() : 0;

		if (rows == 0 || cols == 0) {
			delete grr;
			return;
		}

		// Частоты по X
		double freq_range = display_freq_max - display_freq_min;
		double freq_to_pixel_x = (freq_range != 0) ? plot_width / freq_range : 1.0;

		// Время по Y (ИНВЕРТИРОВАННО: 0 вверху, max внизу)
		double time_range = display_time_max_sec - display_time_min_sec;
		double time_to_pixel_y = (time_range != 0) ? plot_height / time_range : 1.0;

		// Рисуем тепловую карту с правильным соответствием координат
		for (int i = 0; i < rows; ++i) {
			double time_val_sec = display_time_sec[i];

			// Преобразуем время в координату Y (0 вверху, max внизу)
			double normalized_time = time_val_sec - display_time_min_sec;
			double y_display = plot_y + normalized_time * time_to_pixel_y;

			for (int j = 0; j < cols; ++j) {
				double freq_val = display_freq[j];
				double normalized_val = (display_data[i][j] - min_val) / (max_val - min_val);

				int r, g, b;
				GetInfernoColor(normalized_val, r, g, b);
				Gdiplus::SolidBrush br(Gdiplus::Color(255, r, g, b));

				// Преобразуем частоту в координату X
				double normalized_freq = freq_val - display_freq_min;
				double x = plot_x + normalized_freq * freq_to_pixel_x;

				// Вычисляем размеры ячейки
				double cell_width = (j < cols - 1) ?
					(display_freq[j + 1] - freq_val) * freq_to_pixel_x :
					freq_to_pixel_x * (freq_range / cols);

				double cell_height = (i < rows - 1) ?
					(display_time_sec[i + 1] - time_val_sec) * time_to_pixel_y :
					time_to_pixel_y * (time_range / rows);

				// Защита от отрицательных размеров
				cell_width = std::max(1.0, std::abs(cell_width));
				cell_height = std::max(1.0, std::abs(cell_height));

				grr->FillRectangle(&br,
					Gdiplus::REAL(x),
					Gdiplus::REAL(y_display),
					Gdiplus::REAL(cell_width),
					Gdiplus::REAL(cell_height));
			}
		}

		if (show_max_point && (calculated_max_time != 0.0 || calculated_max_freq != 0.0)) {
			// Преобразуем расчетные координаты в экранные координаты
			double target_freq = calculated_max_freq;
			double target_time_sec = calculated_max_time;  // уже в секундах

			// Проверяем, попадают ли расчетные значения в отображаемый диапазон
			bool freq_in_range = (target_freq >= display_freq_min && target_freq <= display_freq_max);
			bool time_in_range = (target_time_sec >= display_time_min_sec && target_time_sec <= display_time_max_sec);

			if (freq_in_range && time_in_range) {
				// Преобразуем в экранные координаты
				double normalized_freq = target_freq - display_freq_min;
				double normalized_time = target_time_sec - display_time_min_sec;

				double center_x = plot_x + normalized_freq * freq_to_pixel_x;
				double center_y = plot_y + normalized_time * time_to_pixel_y;  // ИНВЕРТИРОВАННО

				// Проверяем, что точка внутри области графика
				if (center_x >= plot_x && center_x <= plot_x + plot_width &&
					center_y >= plot_y && center_y <= plot_y + plot_height) {

					Gdiplus::Pen white_pen(Gdiplus::Color::White, 2);

					// Белый крест
					grr->DrawLine(&white_pen,
						Gdiplus::PointF((Gdiplus::REAL)(center_x - 8), (Gdiplus::REAL)center_y),
						Gdiplus::PointF((Gdiplus::REAL)(center_x + 8), (Gdiplus::REAL)center_y));
					grr->DrawLine(&white_pen,
						Gdiplus::PointF((Gdiplus::REAL)center_x, (Gdiplus::REAL)(center_y - 8)),
						Gdiplus::PointF((Gdiplus::REAL)center_x, (Gdiplus::REAL)(center_y + 8)));

					// Подпись с максимальным значением
					double max_val_in_data = 0;
					int max_i = 0, max_j = 0;
					for (int i = 0; i < rows; ++i) {
						for (int j = 0; j < cols; ++j) {
							if (display_data[i][j] > max_val_in_data) {
								max_val_in_data = display_data[i][j];
								max_i = i;
								max_j = j;
							}
						}
					}

					Gdiplus::Font font(L"Arial", 10, Gdiplus::FontStyleBold);
					Gdiplus::SolidBrush text_brush(Gdiplus::Color::White);
					CString label;
					label.Format(L"Max: %.2f", max_val_in_data);

					grr->DrawString(label, -1, &font,
						Gdiplus::PointF((Gdiplus::REAL)(center_x + 10),
							(Gdiplus::REAL)(center_y - 10)),
						&text_brush);

				}
			}
		}

		// Рисуем оси с правильными подписями
		DrawAxesForAmbiguity(*grr, plot_x, plot_y, plot_width, plot_height,
			display_freq_min, display_freq_max,
			display_time_min_sec * 1000.0, display_time_max_sec * 1000.0);  // передаем мс для отображения

		// Цветовая шкала
		DrawColorScale(*grr, min_val, max_val,
			plot_x + plot_width + 10, plot_y, 20, plot_height);

		// Заголовок с информацией
		CString full_title = title;

		DrawTitleAndLabels(*grr, rect.Width(), plot_x, plot_y, plot_width, plot_height,
			full_title, x_label, y_label);

		// Копируем на экран
		gr.DrawImage(&myBitmap, 0, 0, rect.Width(), rect.Height());
		delete grr;
	}

	// Специальная функция для рисования осей функции неопределенности
	void DrawAxesForAmbiguity(Gdiplus::Graphics& grr, int plot_x, int plot_y,
		int plot_width, int plot_height,
		double freq_min, double freq_max,
		double time_min_ms, double time_max_ms)  // время в мс
	{
		Gdiplus::Pen axis_pen(Gdiplus::Color::Black, 2);
		Gdiplus::Pen tick_pen(Gdiplus::Color::Black, 1);

		// Ось X (частота) - внизу
		grr.DrawLine(&axis_pen,
			plot_x, plot_y + plot_height,
			plot_x + plot_width, plot_y + plot_height);

		// Ось Y (время) - слева, инвертирована (0 вверху, max внизу)
		grr.DrawLine(&axis_pen,
			plot_x, plot_y,
			plot_x, plot_y + plot_height);

		// Подписи делений на осях
		Gdiplus::Font font(L"Arial", 10);
		Gdiplus::SolidBrush brush(Gdiplus::Color::Black);
		Gdiplus::StringFormat format;
		format.SetAlignment(Gdiplus::StringAlignmentCenter);

		// Деления оси X (частота) - 5 делений
		int x_ticks = 20;
		for (int i = 0; i <= x_ticks; ++i) {
			double freq = freq_min + i * (freq_max - freq_min) / x_ticks;
			int x_pos = plot_x + i * plot_width / x_ticks;

			// Деление
			grr.DrawLine(&tick_pen, x_pos, plot_y + plot_height - 5,
				x_pos, plot_y + plot_height + 5);

			// Подпись
			CString label;
			if (abs(freq) < 0.1) {
				label.Format(L"%.3f", freq);
			}
			else if (abs(freq) < 1) {
				label.Format(L"%.2f", freq);
			}
			else if (abs(freq) < 10) {
				label.Format(L"%.1f", freq);
			}
			else {
				label.Format(L"%.0f", freq);
			}
			grr.DrawString(label, -1, &font,
				Gdiplus::PointF(x_pos - 20, plot_y + plot_height + 10), &brush);
		}



		// Деления оси Y (время) - инвертированы (0 вверху)
		int y_ticks = 10;
		for (int i = 0; i <= y_ticks; ++i) {
			double time_val = time_min_ms + i * (time_max_ms - time_min_ms) / y_ticks;

			// 0 вверху, максимальное значение внизу
			int y_pos = plot_y + i * plot_height / y_ticks;

			// Деление
			grr.DrawLine(&tick_pen, plot_x - 5, y_pos, plot_x + 5, y_pos);

			// Подпись
			CString label;
			if (abs(time_val) < 0.1) {
				label.Format(L"%.3f", time_val);
			}
			else if (abs(time_val) < 1) {
				label.Format(L"%.2f", time_val);
			}
			else if (abs(time_val) < 10) {
				label.Format(L"%.1f", time_val);
			}
			else {
				label.Format(L"%.0f", time_val);
			}
			grr.DrawString(label, -1, &font,
				Gdiplus::PointF(plot_x - 50, y_pos - 8), &brush);
		}
	}


	void FilterDataForRange(const vector<vector<double>>& data,
		const vector<double>& freq_values,
		const vector<double>& time_values_sec,  // время в секундах
		double freq_min, double freq_max,
		double time_min_sec, double time_max_sec,  // время в секундах
		vector<vector<double>>& filtered_data,
		vector<double>& filtered_freq,
		vector<double>& filtered_time_sec)  // время в секундах
	{
		filtered_data.clear();
		filtered_freq.clear();
		filtered_time_sec.clear();

		// Находим индексы частот в диапазоне
		vector<int> freq_indices;
		for (int i = 0; i < freq_values.size(); ++i) {
			if (freq_values[i] >= freq_min && freq_values[i] <= freq_max) {
				freq_indices.push_back(i);
				filtered_freq.push_back(freq_values[i]);
			}
		}

		// Находим индексы времени в диапазоне (СЕКУНДЫ)
		vector<int> time_indices;
		for (int i = 0; i < time_values_sec.size(); ++i) {
			double time_sec = time_values_sec[i];
			if (time_sec >= time_min_sec && time_sec <= time_max_sec) {
				time_indices.push_back(i);
				filtered_time_sec.push_back(time_sec);
			}
		}

		if (freq_indices.empty() || time_indices.empty()) {
			return;
		}

		// Создаем отфильтрованную матрицу
		filtered_data.resize(time_indices.size());
		for (int i = 0; i < time_indices.size(); ++i) {
			filtered_data[i].resize(freq_indices.size());
			for (int j = 0; j < freq_indices.size(); ++j) {
				filtered_data[i][j] = data[time_indices[i]][freq_indices[j]];
			}
		}
	}

private:

	// Получить цвет по Inferno цветовой карте
	void GetInfernoColor(double normalized_val, int& r, int& g, int& b)
	{
		if (normalized_val < 0.25) {
			r = (int)(64 * normalized_val * 4);
			g = 0;
			b = (int)(255 * normalized_val * 4);
		}
		else if (normalized_val < 0.5) {
			r = (int)(64 + 191 * (normalized_val - 0.25) * 4);
			g = 0;
			b = (int)(255 - 255 * (normalized_val - 0.25) * 4);
		}
		else if (normalized_val < 0.75) {
			r = 255;
			g = (int)(128 * (normalized_val - 0.5) * 4);
			b = 0;
		}
		else {
			r = 255;
			g = (int)(128 + 127 * (normalized_val - 0.75) * 4);
			b = 0;
		}

		// Ограничение значений
		r = min(255, max(0, r));
		g = min(255, max(0, g));
		b = min(255, max(0, b));
	}

	// Рисование осей с подписями
	void DrawAxes(Gdiplus::Graphics& grr, int plot_x, int plot_y,
		int plot_width, int plot_height,
		double freq_min, double freq_max,
		double time_min, double time_max)
	{
		Gdiplus::Pen axis_pen(Gdiplus::Color::Black, 2);
		Gdiplus::Pen tick_pen(Gdiplus::Color::Black, 1);

		// Ось X
		grr.DrawLine(&axis_pen,
			plot_x, plot_y + plot_height,
			plot_x + plot_width, plot_y + plot_height);

		// Ось Y
		grr.DrawLine(&axis_pen,
			plot_x, plot_y,
			plot_x, plot_y + plot_height);

		// Подписи делений на осях
		Gdiplus::Font font(L"Arial", 10);
		Gdiplus::SolidBrush brush(Gdiplus::Color::Black);
		Gdiplus::StringFormat format;
		format.SetAlignment(Gdiplus::StringAlignmentCenter);

		// Деления оси X (частота)
		int x_ticks = 5;
		for (int i = 0; i <= x_ticks; ++i) {
			double freq = freq_min + i * (freq_max - freq_min) / x_ticks;
			int x_pos = plot_x + i * plot_width / x_ticks;

			// Деление
			grr.DrawLine(&tick_pen, x_pos, plot_y + plot_height - 5,
				x_pos, plot_y + plot_height + 5);

			// Подпись
			CString label;
			if (abs(freq) < 0.1) {
				label.Format(L"%.3f", freq);
			}
			else if (abs(freq) < 1) {
				label.Format(L"%.2f", freq);
			}
			else if (abs(freq) < 10) {
				label.Format(L"%.1f", freq);
			}
			else {
				label.Format(L"%.0f", freq);
			}
			grr.DrawString(label, -1, &font,
				Gdiplus::PointF(x_pos - 20, plot_y + plot_height + 10), &brush);
		}

		// Деления оси Y (время)
		int y_ticks = 5;
		for (int i = 0; i <= y_ticks; ++i) {
			double time_val = time_min + i * (time_max - time_min) / y_ticks;
			int y_pos = plot_y + plot_height - i * plot_height / y_ticks;

			// Деление
			grr.DrawLine(&tick_pen, plot_x - 5, y_pos, plot_x + 5, y_pos);

			// Подпись
			CString label;
			if (abs(time_val) < 0.1) {
				label.Format(L"%.3f", time_val);
			}
			else if (abs(time_val) < 1) {
				label.Format(L"%.2f", time_val);
			}
			else if (abs(time_val) < 10) {
				label.Format(L"%.1f", time_val);
			}
			else {
				label.Format(L"%.0f", time_val);
			}
			grr.DrawString(label, -1, &font,
				Gdiplus::PointF(plot_x - 50, y_pos - 8), &brush);
		}
	}

	// Рисование цветовой шкалы
	void DrawColorScale(Gdiplus::Graphics& grr, double min_val, double max_val,
		int x, int y, int width, int height)
	{
		int steps = 256;
		double step_height = height / (double)steps;

		for (int i = 0; i < steps; ++i) {
			double normalized_val = i / (double)steps;

			int r, g, b;
			GetInfernoColor(normalized_val, r, g, b);

			Gdiplus::SolidBrush br(Gdiplus::Color(255, r, g, b));
			int y1 = y + i * step_height;
			int y2 = y + (i + 1) * step_height;

			grr.FillRectangle(&br, x, y1, width, y2 - y1);
		}

		// Рамка
		Gdiplus::Pen border_pen(Gdiplus::Color::Black, 1);
		grr.DrawRectangle(&border_pen, x, y, width, height);

		// Подписи значений
		Gdiplus::Font font(L"Arial", 10);
		Gdiplus::SolidBrush brush(Gdiplus::Color::Black);

		CString label;
		label.Format(L"%.3f", min_val);
		grr.DrawString(label, -1, &font,
			Gdiplus::PointF(x + width + 5, y - 15), &brush);

		label.Format(L"%.3f", max_val);
		grr.DrawString(label, -1, &font,
			Gdiplus::PointF(x + width + 5, y + height - 15), &brush);
	}

	// Рисование заголовка и подписей
	void DrawTitleAndLabels(Gdiplus::Graphics& grr, int total_width,
		int plot_x, int plot_y, int plot_width, int plot_height,
		const CString& title, const CString& x_label, const CString& y_label)
	{
		// Заголовок
		if (!title.IsEmpty()) {
			Gdiplus::Font title_font(L"Arial", 14, Gdiplus::FontStyleBold);
			Gdiplus::SolidBrush title_brush(Gdiplus::Color::Black);

			Gdiplus::StringFormat title_format;
			title_format.SetAlignment(Gdiplus::StringAlignmentCenter);

			grr.DrawString(title, -1, &title_font,
				Gdiplus::RectF(0, 5, total_width, 30),
				&title_format, &title_brush);
		}

		// Подпись оси X
		if (!x_label.IsEmpty()) {
			Gdiplus::Font font(L"Arial", 12);
			Gdiplus::SolidBrush brush(Gdiplus::Color::Black);

			Gdiplus::StringFormat format;
			format.SetAlignment(Gdiplus::StringAlignmentCenter);

			grr.DrawString(x_label, -1, &font,
				Gdiplus::RectF(plot_x, plot_y + plot_height + 22, plot_width, 20),
				&format, &brush);
		}

		// Подпись оси Y
		if (!y_label.IsEmpty()) {
			Gdiplus::Font font(L"Arial", 12);
			Gdiplus::SolidBrush brush(Gdiplus::Color::Black);

			Gdiplus::StringFormat y_format;
			y_format.SetAlignment(Gdiplus::StringAlignmentCenter);

			grr.DrawString(y_label, -1, &font,
				Gdiplus::RectF(-0.35 * total_width, 10, plot_width, 20),
				&y_format, &brush);

			grr.ResetTransform();
		}
	}

	//// Вспомогательные функции для преобразования диапазонов
	//double convert_range(double value, double in_min, double in_max, double out_min, double out_max)
	//{
	//	return (value - in_min) * (out_max - out_min) / (in_max - in_min) + out_min;
	//}

	//vector<double> convert_range(vector<double>& data, double outmax, double outmin,
	//	double inmax, double inmin)
	//{
	//	vector<double> output = data;
	//	double k = (outmax - outmin) / (inmax - inmin);
	//	for (auto& item : output)
	//	{
	//		item = (item - inmin) * k + outmin;
	//	}
	//	return output;
	//}

public:
	// Нарисовать график по переданным данным.
	void Draw(vector<double>& data, double min_data, double max_data,
		vector<double>& keys, double min_keys, double max_keys,
		char color, CString name_x, CString name_y)
	{
		if (!init) return;
		CPen subgrid_pen(PS_DOT, 1, RGB(200, 200, 200));
		CPen grid_pen(PS_SOLID, 1, RGB(0, 0, 0));
		CPen data_pen(PS_SOLID, 2, RGB(255, 0, 0));
		CPen data_pen2(PS_SOLID, 2, RGB(38, 0, 255));
		CPen pen_red(PS_SOLID, 2, RGB(178, 34, 34));
		CPen pen_green(PS_SOLID, 2, RGB(0, 128, 0));
		CFont font;
		font.CreateFontW(18, 0, 0, 0,
			FW_DONTCARE,
			FALSE,				// Курсив
			FALSE,				// Подчеркнутый
			FALSE,				// Перечеркнутый
			DEFAULT_CHARSET,	// Набор символов
			OUT_OUTLINE_PRECIS,	// Точность соответствия.	
			CLIP_DEFAULT_PRECIS,//  
			CLEARTYPE_QUALITY,	// Качество
			VARIABLE_PITCH,		//
			TEXT("Times New Roman")		//
		);

		int padding = 20;
		int left_keys_padding = 20;
		int bottom_keys_padding = 10;

		int actual_width = frame.Width() - 2 * padding - left_keys_padding;
		int actual_height = frame.Height() - 2 * padding - bottom_keys_padding;

		int actual_top = padding;
		int actual_bottom = actual_top + actual_height;
		int actual_left = padding + left_keys_padding;
		int actual_right = actual_left + actual_width;

		// Белый фон.
		memDC.FillSolidRect(frame, RGB(255, 255, 255));

		// Рисуем сетку и подсетку.
		unsigned int grid_size = 10;

		memDC.SelectObject(&subgrid_pen);

		for (double i = 0.5; i < grid_size; i += 1.0)
		{
			memDC.MoveTo(actual_left + i * actual_width / grid_size, actual_top);
			memDC.LineTo(actual_left + i * actual_width / grid_size, actual_bottom);
			memDC.MoveTo(actual_left, actual_top + i * actual_height / grid_size);
			memDC.LineTo(actual_right, actual_top + i * actual_height / grid_size);
		}

		memDC.SelectObject(&grid_pen);

		for (double i = 0.0; i < grid_size + 1; i += 1.0)
		{
			memDC.MoveTo(actual_left + i * actual_width / grid_size, actual_top);
			memDC.LineTo(actual_left + i * actual_width / grid_size, actual_bottom);
			memDC.MoveTo(actual_left, actual_top + i * actual_height / grid_size);
			memDC.LineTo(actual_right, actual_top + i * actual_height / grid_size);
		}

		// Рисуем график.
		if (data.empty()) return;

		if (keys.size() != data.size())
		{
			keys.resize(data.size());
			for (int i = 0; i < keys.size(); i++)
			{
				keys[i] = i;
			}
		}

		if (color == 'R')memDC.SelectObject(&pen_red);
		else if (color == 'G')memDC.SelectObject(&pen_green);
		else memDC.SelectObject(&data_pen);

		double data_y_max(max_data), data_y_min(min_data);
		double data_x_max(max_keys), data_x_min(min_keys);

		vector<double> y = convert_range(data, actual_top, actual_bottom, data_y_max, data_y_min);
		vector<double> x = convert_range(keys, actual_right, actual_left, data_x_max, data_x_min);

		memDC.MoveTo(x[0], y[0]);
		for (unsigned int i = 0; i < y.size(); i++)
		{
			memDC.LineTo(x[i], y[i]);
		}

		memDC.SelectObject(&font);
		memDC.SetTextColor(RGB(0, 0, 0));
		for (int i = 0; i < grid_size; i++)
		{
			CString str;
			str.Format(L"%.1f", data_x_min + i * (data_x_max - data_x_min) / (grid_size / 1));
			memDC.TextOutW(actual_left + (double)i * actual_width / (grid_size / 1) - bottom_keys_padding, actual_bottom + bottom_keys_padding / 2, str);
		}

		for (int i = 0; i < grid_size / 2; i++)
		{
			CString str;
			str.Format(L"%.1f", data_y_min + i * (data_y_max - data_y_min) / (grid_size / 2));
			memDC.TextOutW(actual_left - 1.5 * left_keys_padding, actual_bottom - (double)i * actual_height / (grid_size / 2) - bottom_keys_padding, str);
		}

		CString str;
		str = name_x;
		memDC.TextOutW(actual_left - 10 + (grid_size / 2) * actual_width / (grid_size / 2) - bottom_keys_padding, actual_bottom + bottom_keys_padding / 2, str);


		str = name_y;
		memDC.TextOutW(actual_left - 1.5 * left_keys_padding, actual_bottom - grid_size / 2 * actual_height / (grid_size / 2) - bottom_keys_padding, str);

		dc->BitBlt(0, 0, frame.Width(), frame.Height(), &memDC, 0, 0, SRCCOPY);
	}

	void Draw2(vector<double>& data, double min_data, double max_data,
		vector<double>& keys, double min_keys, double max_keys,
		char color,
		vector<double>& data2,
		vector<double>& keys2,
		char color2, CString name_x, CString name_y
	)
	{
		if (!init) return;

		CPen subgrid_pen(PS_DOT, 1, RGB(200, 200, 200));
		CPen grid_pen(PS_SOLID, 1, RGB(0, 0, 0));
		CPen data_pen(PS_SOLID, 2, RGB(255, 0, 0));
		CPen data_pen2(PS_SOLID, 2, RGB(38, 0, 255));
		CPen pen_red(PS_SOLID, 2, RGB(178, 34, 34));
		CPen pen_green(PS_SOLID, 2, RGB(0, 128, 0));
		CFont font;
		font.CreateFontW(18, 0, 0, 0,
			FW_DONTCARE,
			FALSE,				// Курсив
			FALSE,				// Подчеркнутый
			FALSE,				// Перечеркнутый
			DEFAULT_CHARSET,	// Набор символов
			OUT_OUTLINE_PRECIS,	// Точность соответствия.	
			CLIP_DEFAULT_PRECIS,//  
			CLEARTYPE_QUALITY,	// Качество
			VARIABLE_PITCH,		//
			TEXT("Times New Roman")		//
		);

		int padding = 20;
		int left_keys_padding = 20;
		int bottom_keys_padding = 10;

		int actual_width = frame.Width() - 2 * padding - left_keys_padding;
		int actual_height = frame.Height() - 2 * padding - bottom_keys_padding;

		int actual_top = padding;
		int actual_bottom = actual_top + actual_height;
		int actual_left = padding + left_keys_padding;
		int actual_right = actual_left + actual_width;

		// Белый фон.
		memDC.FillSolidRect(frame, RGB(255, 255, 255));

		// Рисуем сетку и подсетку.
		unsigned int grid_size = 10;

		memDC.SelectObject(&subgrid_pen);

		for (double i = 0.5; i < grid_size; i += 1.0)
		{
			memDC.MoveTo(actual_left + i * actual_width / grid_size, actual_top);
			memDC.LineTo(actual_left + i * actual_width / grid_size, actual_bottom);
			memDC.MoveTo(actual_left, actual_top + i * actual_height / grid_size);
			memDC.LineTo(actual_right, actual_top + i * actual_height / grid_size);
		}

		memDC.SelectObject(&grid_pen);

		for (double i = 0.0; i < grid_size + 1; i += 1.0)
		{
			memDC.MoveTo(actual_left + i * actual_width / grid_size, actual_top);
			memDC.LineTo(actual_left + i * actual_width / grid_size, actual_bottom);
			memDC.MoveTo(actual_left, actual_top + i * actual_height / grid_size);
			memDC.LineTo(actual_right, actual_top + i * actual_height / grid_size);
		}

		// Рисуем график.
		if (data.empty()) return;

		if (keys.size() != data.size())
		{
			keys.resize(data.size());
			for (int i = 0; i < keys.size(); i++)
			{
				keys[i] = i;
			}
		}

		/*РИСУЕМ ПЕРВЫЙ ГРАФИК*/
		if (color == 'R')memDC.SelectObject(&pen_red);
		else if (color == 'G')memDC.SelectObject(&pen_green);
		else memDC.SelectObject(&data_pen);

		double data_y_max(max_data), data_y_min(min_data);
		double data_x_max(max_keys), data_x_min(min_keys);

		vector<double> y = convert_range(data, actual_top, actual_bottom, data_y_max, data_y_min);
		vector<double> x = convert_range(keys, actual_right, actual_left, data_x_max, data_x_min);

		memDC.MoveTo(x[0], y[0]);
		for (unsigned int i = 0; i < y.size(); i++)
		{
			memDC.LineTo(x[i], y[i]);
		}
		/*РИСУЕМ ВТОРОЙ ГРАФИК*/
		if (color2 == 'R')memDC.SelectObject(&pen_red);
		else if (color2 == 'G')memDC.SelectObject(&pen_green);
		else memDC.SelectObject(&data_pen);

		y = convert_range(data2, actual_top, actual_bottom, data_y_max, data_y_min);
		x = convert_range(keys2, actual_right, actual_left, data_x_max, data_x_min);

		memDC.MoveTo(x[0], y[0]);
		for (unsigned int i = 0; i < y.size(); i++)
		{
			memDC.LineTo(x[i], y[i]);
		}
		/**/

		memDC.SelectObject(&font);
		memDC.SetTextColor(RGB(0, 0, 0));
		for (int i = 0; i < grid_size / 2 + 1; i++)
		{
			CString str;
			str.Format(L"%.1f", data_x_min + i * (data_x_max - data_x_min) / (grid_size / 2));
			memDC.TextOutW(actual_left + (double)i * actual_width / (grid_size / 2) - bottom_keys_padding, actual_bottom + bottom_keys_padding / 2, str);

			str.Format(L"%.1f", data_y_min + i * (data_y_max - data_y_min) / (grid_size / 2));
			memDC.TextOutW(actual_left - 1.5 * left_keys_padding, actual_bottom - (double)i * actual_height / (grid_size / 2) - bottom_keys_padding, str);
		}
		CString str;
		str = name_x;
		memDC.TextOutW(actual_left - 10 + (grid_size / 2) * actual_width / (grid_size / 2) - bottom_keys_padding, actual_bottom + bottom_keys_padding / 2, str);


		str = name_y;
		memDC.TextOutW(actual_left - 1.5 * left_keys_padding, actual_bottom - grid_size / 2 * actual_height / (grid_size / 2) - bottom_keys_padding, str);

		dc->BitBlt(0, 0, frame.Width(), frame.Height(), &memDC, 0, 0, SRCCOPY);
	}

	void Draw3(vector<double>& data1, double min_data, double max_data,
		vector<double>& keys1, double min_keys, double max_keys,
		char color1,
		vector<double>& data2,
		vector<double>& keys2,
		char color2,
		vector<double>& data3,
		vector<double>& keys3,
		char color3, CString name_x, CString name_y)
	{
		if (!init) return;

		CPen subgrid_pen(PS_DOT, 1, RGB(200, 200, 200));
		CPen grid_pen(PS_SOLID, 1, RGB(0, 0, 0));
		CPen data_pen(PS_SOLID, 2, RGB(255, 0, 0));
		CPen data_pen2(PS_SOLID, 2, RGB(38, 0, 255));
		CPen pen_red(PS_SOLID, 2, RGB(178, 34, 34));
		CPen pen_green(PS_SOLID, 2, RGB(0, 128, 0));
		CFont font;
		font.CreateFontW(18, 0, 0, 0,
			FW_DONTCARE,
			FALSE,              // Курсив
			FALSE,              // Подчеркнутый
			FALSE,              // Перечеркнутый
			DEFAULT_CHARSET,    // Набор символов
			OUT_OUTLINE_PRECIS, // Точность соответствия.    
			CLIP_DEFAULT_PRECIS,//  
			CLEARTYPE_QUALITY,  // Качество
			VARIABLE_PITCH,     //
			TEXT("Times New Roman")     //
		);

		int padding = 20;
		int left_keys_padding = 20;
		int bottom_keys_padding = 10;

		int actual_width = frame.Width() - 2 * padding - left_keys_padding;
		int actual_height = frame.Height() - 2 * padding - bottom_keys_padding;

		int actual_top = padding;
		int actual_bottom = actual_top + actual_height;
		int actual_left = padding + left_keys_padding;
		int actual_right = actual_left + actual_width;

		// Белый фон.
		memDC.FillSolidRect(frame, RGB(255, 255, 255));

		// Рисуем сетку и подсетку.
		unsigned int grid_size = 10;

		memDC.SelectObject(&subgrid_pen);

		for (double i = 0.5; i < grid_size; i += 1.0)
		{
			memDC.MoveTo(actual_left + i * actual_width / grid_size, actual_top);
			memDC.LineTo(actual_left + i * actual_width / grid_size, actual_bottom);
			memDC.MoveTo(actual_left, actual_top + i * actual_height / grid_size);
			memDC.LineTo(actual_right, actual_top + i * actual_height / grid_size);
		}

		memDC.SelectObject(&grid_pen);

		for (double i = 0.0; i < grid_size + 1; i += 1.0)
		{
			memDC.MoveTo(actual_left + i * actual_width / grid_size, actual_top);
			memDC.LineTo(actual_left + i * actual_width / grid_size, actual_bottom);
			memDC.MoveTo(actual_left, actual_top + i * actual_height / grid_size);
			memDC.LineTo(actual_right, actual_top + i * actual_height / grid_size);
		}

		// Рисуем графики
		if (data1.empty() && data2.empty() && data3.empty()) return;

		/*РИСУЕМ ПЕРВЫЙ ГРАФИК*/
		if (color1 == 'R') memDC.SelectObject(&pen_red);
		else if (color1 == 'G') memDC.SelectObject(&pen_green);
		else if (color1 == 'B') memDC.SelectObject(&data_pen2); // Синий
		else memDC.SelectObject(&data_pen); // Красный по умолчанию

		if (!data1.empty() && !keys1.empty()) {
			// Проверяем размеры
			if (keys1.size() != data1.size()) {
				keys1.resize(data1.size());
				for (size_t i = 0; i < keys1.size(); i++) {
					keys1[i] = i;
				}
			}

			vector<double> y = convert_range(data1, actual_top, actual_bottom, max_data, min_data);
			vector<double> x = convert_range(keys1, actual_right, actual_left, max_keys, min_keys);

			if (!x.empty() && !y.empty()) {
				memDC.MoveTo(x[0], y[0]);
				for (size_t i = 0; i < y.size(); i++) {
					memDC.LineTo(x[i], y[i]);
				}
			}
		}

		if (color2 == 'R') memDC.SelectObject(&pen_red);
		else if (color2 == 'G') memDC.SelectObject(&pen_green);
		else if (color2 == 'B') memDC.SelectObject(&data_pen2);
		else memDC.SelectObject(&data_pen);

		if (!data2.empty() && !keys2.empty()) {
			// Проверяем размеры
			if (keys2.size() != data2.size()) {
				keys2.resize(data2.size());
				for (size_t i = 0; i < keys2.size(); i++) {
					keys2[i] = i;
				}
			}

			vector<double> y = convert_range(data2, actual_top, actual_bottom, max_data, min_data);
			vector<double> x = convert_range(keys2, actual_right, actual_left, max_keys, min_keys);

			if (!x.empty() && !y.empty()) {
				memDC.MoveTo(x[0], y[0]);
				for (size_t i = 0; i < y.size(); i++) {
					memDC.LineTo(x[i], y[i]);
				}
			}
		}

		if (color3 == 'R') memDC.SelectObject(&pen_red);
		else if (color3 == 'G') memDC.SelectObject(&pen_green);
		else if (color3 == 'B') memDC.SelectObject(&data_pen2);
		else memDC.SelectObject(&data_pen);

		if (!data3.empty() && !keys3.empty()) {
			// Проверяем размеры
			if (keys3.size() != data3.size()) {
				keys3.resize(data3.size());
				for (size_t i = 0; i < keys3.size(); i++) {
					keys3[i] = i;
				}
			}

			vector<double> y = convert_range(data3, actual_top, actual_bottom, max_data, min_data);
			vector<double> x = convert_range(keys3, actual_right, actual_left, max_keys, min_keys);

			if (!x.empty() && !y.empty()) {
				memDC.MoveTo(x[0], y[0]);
				for (size_t i = 0; i < y.size(); i++) {
					memDC.LineTo(x[i], y[i]);
				}
			}
		}

		memDC.SelectObject(&font);
		memDC.SetTextColor(RGB(0, 0, 0));
		for (int i = 0; i < grid_size / 2 + 1; i++)
		{
			CString str;
			str.Format(L"%.1f", min_keys + i * (max_keys - min_keys) / (grid_size / 2));
			memDC.TextOutW(actual_left + (double)i * actual_width / (grid_size / 2) - bottom_keys_padding, actual_bottom + bottom_keys_padding / 2, str);

			str.Format(L"%.1f", min_data + i * (max_data - min_data) / (grid_size / 2));
			memDC.TextOutW(actual_left - 1.5 * left_keys_padding, actual_bottom - (double)i * actual_height / (grid_size / 2) - bottom_keys_padding, str);
		}
		CString str;
		str = name_x;
		memDC.TextOutW(actual_left - 10 + (grid_size / 2) * actual_width / (grid_size / 2) - bottom_keys_padding, actual_bottom + bottom_keys_padding / 2, str);


		str = name_y;
		memDC.TextOutW(actual_left - 1.5 * left_keys_padding, actual_bottom - grid_size / 2 * actual_height / (grid_size / 2) - bottom_keys_padding, str);

		dc->BitBlt(0, 0, frame.Width(), frame.Height(), &memDC, 0, 0, SRCCOPY);
	}

	vector<double> convert_range(vector <double>& data, double outmax, double outmin, double inmax, double inmin)
	{
		vector<double> output = data;
		double k = (outmax - outmin) / (inmax - inmin);
		for (auto& item : output)
		{
			item = (item - inmin) * k + outmin;
		}

		return output;
	}
};