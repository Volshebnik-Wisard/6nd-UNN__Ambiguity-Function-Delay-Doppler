#include "pch.h"
#include "framework.h"
#include "ambiguity_function.h"
#include "ambiguity_functionDlg.h"
#include "afxdialogex.h"

#undef min
#undef max

#ifdef _DEBUG
#define new DEBUG_NEW
#endif

// Глобальная переменная для шага сглаживания
int stepff;

// Конструктор класса Cambiguity_functionDlg
Cambiguity_functionDlg::Cambiguity_functionDlg(CWnd* pParent /*=nullptr*/)
	: CDialogEx(IDD_CHECK_DELAY_DIALOG, pParent)
	, fd(2.003)           // Частота дискретизации по умолчанию (в МГц)
	, nbits(400)          // Количество битов по умолчанию
	, bitrate(400)        // Битрейт по умолчанию (в бит/с)
	, fc(100)             // Несущая частота по умолчанию (в Гц)
	, delay(100)          // Задержка по умолчанию (в мс)
	, snr(15)             // ОСШ для базового сигнала по умолчанию (в дБ)
	, result_delay(_T("")) // Строка результата оценки задержки
	, sample_base(400)    // Длительность базового сигнала по умолчанию (в отсчетах)
	, snr_fully(15)       // ОСШ для полного сигнала по умолчанию (в дБ)
	, min_snr(10)         // Минимальное ОСШ для экспериментов (в дБ)
	, max_snr(1000)       // Максимальное ОСШ для экспериментов (в дБ)
	, step_snr(3)         // Шаг изменения ОСШ для экспериментов
	, N_generate(200)     // Количество генераций для усреднения в экспериментах
	, m_customFreqMin(-100) // Минимальная частота для отображения ФН
	, m_customFreqMax(300)  // Максимальная частота для отображения ФН
	, m_customTimeMin(0)    // Минимальное время для отображения ФН
	, m_customTimeMax(600)  // Максимальное время для отображения ФН
	, m_customLevels(0)     // Количество уровней изолиний для ФН
{
	// Загрузка иконки приложения
	m_hIcon = AfxGetApp()->LoadIcon(IDR_MAINFRAME);
}

// Функция обмена данными между элементами управления и переменными
void Cambiguity_functionDlg::DoDataExchange(CDataExchange* pDX)
{
	CDialogEx::DoDataExchange(pDX);
	// Связывание переменных с элементами редактирования
	DDX_Text(pDX, IDC_EDIT_FD, fd);
	DDX_Text(pDX, IDC_EDIT_NBITS, nbits);
	DDX_Text(pDX, IDC_EDIT_BITRATE, bitrate);
	DDX_Text(pDX, IDC_EDIT_FC, fc);
	DDX_Text(pDX, IDC_EDIT_DELAY, delay);
	DDX_Text(pDX, IDC_EDIT_SNR, snr);
	DDX_Text(pDX, IDC_STATIC_EST_DELAY, result_delay);
	DDX_Text(pDX, IDC_EDIT_SAMPLE_BASE, sample_base);
	DDX_Text(pDX, IDC_EDIT_SNR2, snr_fully);
	DDX_Text(pDX, IDC_EDIT_DELAY2, min_snr);
	DDX_Text(pDX, IDC_EDIT_SAMPLE_BASE2, max_snr);
	DDX_Text(pDX, IDC_EDIT_SNR3, step_snr);
	DDX_Text(pDX, IDC_EDIT_SNR4, N_generate);
	// Связывание элементов управления
	DDX_Control(pDX, IDC_PROGRESS1, progress_experement);
	DDX_Control(pDX, IDC_GRAPH5, m_picCodeSequence);
	DDX_Control(pDX, IDC_GRAPH2, m_picCtrlFullSignal);
	DDX_Control(pDX, IDC_GRAPH1, m_picCtrlCorrelation);
	DDX_Control(pDX, IDC_GRAPH4, m_picCtrlExperiment);
	// Связывание параметров для отображения ФН
	DDX_Text(pDX, IDC_EDIT1, m_customFreqMin);
	DDX_Text(pDX, IDC_EDIT2, m_customFreqMax);
	DDX_Text(pDX, IDC_EDIT3, m_customTimeMin);
	DDX_Text(pDX, IDC_EDIT4, m_customTimeMax);
	DDX_Text(pDX, IDC_EDIT5, m_customLevels);
}

// Карта сообщений (связывает сообщения с функциями-обработчиками)
BEGIN_MESSAGE_MAP(Cambiguity_functionDlg, CDialogEx)
	ON_WM_PAINT()  // Обработчик сообщения WM_PAINT (перерисовка окна)
	ON_WM_QUERYDRAGICON()  // Обработчик запроса иконки для перетаскивания
	ON_BN_CLICKED(IDC_BUTCREATE, &Cambiguity_functionDlg::OnBnClickedButcreate)  // Кнопка "Создать"
	ON_BN_CLICKED(IDC_RADIO_AM, &Cambiguity_functionDlg::OnBnClickedRadioAm)     // Радиокнопка AM
	ON_BN_CLICKED(IDC_RADIO_PM, &Cambiguity_functionDlg::OnBnClickedRadioPm)     // Радиокнопка PM
	ON_BN_CLICKED(IDC_RADIO_FRM, &Cambiguity_functionDlg::OnBnClickedRadioFrm)   // Радиокнопка FM
	ON_BN_CLICKED(IDC_BUTTON_ESTIMATE, &Cambiguity_functionDlg::OnBnClickedButtonEstimate)  // Кнопка "Оценить"
	ON_BN_CLICKED(IDC_BUTTON1, &Cambiguity_functionDlg::OnBnClickedButton1)      // Кнопка "ФН" (горячая)
	ON_BN_CLICKED(IDC_BUTTON3, &Cambiguity_functionDlg::OnBnClickedButton3)      // Кнопка "ФН" (контур)
END_MESSAGE_MAP()

// Инициализация диалогового окна
BOOL Cambiguity_functionDlg::OnInitDialog()
{
	CDialogEx::OnInitDialog();
	// Установка иконки окна
	SetIcon(m_hIcon, TRUE);   // Установка большой иконки
	SetIcon(m_hIcon, FALSE);  // Установка маленькой иконки

	// Инициализация объектов Drawer для отрисовки графиков
	// drv1 для графика корреляции (IDC_GRAPH1)
	drv1.Create(GetDlgItem(IDC_GRAPH1)->GetSafeHwnd());
	// drv2 для графика полного сигнала (IDC_GRAPH2)
	drv2.Create(GetDlgItem(IDC_GRAPH2)->GetSafeHwnd());
	// drv4 для графика экспериментов (IDC_GRAPH4)
	drv4.Create(GetDlgItem(IDC_GRAPH4)->GetSafeHwnd());
	// drv5 для графика кодовой последовательности (IDC_GRAPH5)
	drv5.Create(GetDlgItem(IDC_GRAPH5)->GetSafeHwnd());

	// Инициализация прогресс-бара для экспериментов
	progress_experement.SetRange(0, 100);  // Установка диапазона от 0 до 100%

	return TRUE;  // Возврат TRUE для фокусировки на первом элементе управления
}

// Обработчик запроса иконки для перетаскивания окна
HCURSOR Cambiguity_functionDlg::OnQueryDragIcon()
{
	return static_cast<HCURSOR>(m_hIcon);  // Возврат курсора для перетаскивания
}

// Обработчик перерисовки окна
void Cambiguity_functionDlg::OnPaint()
{
	// Если окно свернуто (иконка)
	if (IsIconic())
	{
		CPaintDC dc(this);  // Контекст устройства для рисования
		SendMessage(WM_ICONERASEBKGND, reinterpret_cast<WPARAM>(dc.GetSafeHdc()), 0);  // Стирание фона иконки

		// Получение размеров иконки
		int cxIcon = GetSystemMetrics(SM_CXICON);
		int cyIcon = GetSystemMetrics(SM_CYICON);
		CRect rect;
		GetClientRect(&rect);  // Получение размеров клиентской области
		// Вычисление центра для отрисовки иконки
		int x = (rect.Width() - cxIcon + 1) / 2;
		int y = (rect.Height() - cyIcon + 1) / 2;

		dc.DrawIcon(x, y, m_hIcon);  // Отрисовка иконки
	}
	else
	{
		CDialogEx::OnPaint();  // Стандартная обработка перерисовки
	}
}


// Функция для симметричного сглаживания с учетом резкого начального спада
void smoothSymmetricWithInitialDrop(std::vector<double>& values,
	const std::vector<double>& x_axis,
	int window_size = 100)  // Размер окна сглаживания по умолчанию
{
	// Проверка корректности входных данных
	if (values.size() < 3 || x_axis.size() != values.size()) return;

	std::vector<double> smoothed(values.size());  // Вектор для сглаженных значений

	// 1. Находим индекс нуля (или ближайшего к нулю) на оси X
	int zero_index = 0;
	double min_abs = fabs(x_axis[0]);  // Минимальное абсолютное значение
	for (size_t i = 1; i < x_axis.size(); i++) {
		if (fabs(x_axis[i]) < min_abs) {
			min_abs = fabs(x_axis[i]);
			zero_index = i;  // Обновление индекса нуля
		}
	}

	// 2. Сначала применяем обычное скользящее среднее
	for (size_t i = 0; i < values.size(); i++) {
		double sum = 0.0;  // Сумма значений в окне
		int count = 0;      // Количество точек в окне

		// Адаптивный размер окна: меньше для начальных точек
		int adaptive_window = window_size / stepff / stepff;  // Адаптация по шагу
		if (i < 3 || i > values.size() - 4) {
			adaptive_window = 3;  // Уже окно по краям
		}

		// Вычисление скользящего среднего
		for (int j = -adaptive_window / 2; j <= adaptive_window / 2; j++) {
			int idx = static_cast<int>(i) + j;  // Индекс с учетом сдвига
			if (idx >= 0 && idx < static_cast<int>(values.size())) {
				sum += values[idx];  // Добавление значения
				count++;             // Увеличение счетчика
			}
		}
		smoothed[i] = sum / count;  // Усреднение
	}

	// 3. Обеспечиваем симметрию относительно нуля
	for (size_t i = 0; i <= zero_index; i++) {
		size_t sym_idx = 2 * zero_index - i;  // Индекс симметричной точки
		if (sym_idx < values.size()) {
			// Усредняем симметричные точки
			double avg = (smoothed[i] + smoothed[sym_idx]) / 2.0;
			smoothed[i] = avg;
			smoothed[sym_idx] = avg;
		}
	}

	// 4. Обрабатываем резкий начальный спад (первые 5-10% точек)
	int drop_end_idx = values.size() * 0.1;  // 10% точек от начала
	if (drop_end_idx > zero_index) drop_end_idx = zero_index;  // Корректировка, если вышли за ноль

	// Проверяем, есть ли реальный резкий спад
	if (drop_end_idx > 3) {
		double initial_value = smoothed[0];            // Начальное значение
		double after_drop_value = smoothed[drop_end_idx];  // Значение после спада
		double drop_ratio = after_drop_value / initial_value;  // Отношение спада

		// Если спад действительно резкий (значение упало более чем в 2 раза)
		if (drop_ratio < 0.5 && initial_value > 0) {
			// Создаем плавную экспоненциальную кривую спада
			double decay_factor = pow(drop_ratio, 1.0 / drop_end_idx);  // Коэффициент затухания

			for (int i = 0; i <= drop_end_idx; i++) {
				double target_value = initial_value * pow(decay_factor, i);  // Целевое значение
				// Смешиваем с отфильтрованным значением
				double mix_ratio = (double)i / drop_end_idx;  // Коэффициент смешивания
				smoothed[i] = target_value * (1.0 - mix_ratio) + smoothed[i] * mix_ratio;  // Линейная интерполяция
			}
		}
	}

	// 5. Применяем монотонное убывание от максимума к минимуму
	// Находим глобальный максимум (обычно в точке 0 или около нее)
	int max_idx = zero_index;  // Предполагаем, что максимум в нуле
	for (int i = max(0, zero_index - 3); i <= min((int)values.size() - 1, zero_index + 3); i++) {
		if (smoothed[i] > smoothed[max_idx]) {
			max_idx = i;  // Обновление индекса максимума
		}
	}

	// Убывание вправо от максимума (не позволяем значениям расти)
	for (size_t i = max_idx + 1; i < values.size(); i++) {
		if (smoothed[i] > smoothed[i - 1] * 0.99) { // Допускаем небольшой рост 1%
			smoothed[i] = smoothed[i - 1] * 0.99;  // Ограничение роста
		}
	}

	// Убывание влево от максимума
	for (int i = max_idx - 1; i >= 0; i--) {
		if (smoothed[i] > smoothed[i + 1] * 0.99) {
			smoothed[i] = smoothed[i + 1] * 0.99;  // Ограничение роста
		}
	}

	// 6. Устраняем "бугорки" от резких переходов при большом шаге
	// Ищем резкие локальные максимумы
	for (size_t i = 1; i < values.size() - 1; i++) {
		// Если точка значительно выше соседей и это не глобальный максимум
		if (smoothed[i] > smoothed[i - 1] * 1.2 &&
			smoothed[i] > smoothed[i + 1] * 1.2 &&
			abs((int)i - max_idx) > window_size) {

			// Сглаживаем бугорок
			double avg_neighbors = (smoothed[i - 1] + smoothed[i + 1]) / 2.0;  // Среднее соседей
			smoothed[i] = avg_neighbors * 0.7 + smoothed[i] * 0.3;  // Взвешенное усреднение

			// Также сглаживаем соседние точки для плавности
			if (i > 1) smoothed[i - 1] = (smoothed[i - 2] + avg_neighbors) / 2.0;
			if (i < values.size() - 2) smoothed[i + 1] = (smoothed[i + 2] + avg_neighbors) / 2.0;
		}
	}

	// 7. Финальное легкое сглаживание
	std::vector<double> final_smoothed = smoothed;  // Копия для финального сглаживания
	for (size_t i = 1; i < values.size() - 1; i++) {
		// Простое сглаживание по трем точкам
		final_smoothed[i] = (smoothed[i - 1] + smoothed[i] * 2 + smoothed[i + 1]) / 4.0;
	}

	values = final_smoothed;  // Замена исходных значений сглаженными
}

// Улучшенная версия для нескольких кривых с сохранением их относительного положения
void smoothAllCurvesSymmetric(std::vector<double>& am_values,
	std::vector<double>& pm_values,
	std::vector<double>& fm_values,
	const std::vector<double>& x_axis)
{
	// Проверка корректности размеров векторов
	if (am_values.size() != pm_values.size() ||
		pm_values.size() != fm_values.size() ||
		am_values.size() != x_axis.size()) {
		return;  // Выход при несоответствии размеров
	}

	// Сохраняем оригинальные значения для сохранения пропорций
	std::vector<double> am_original = am_values;  // Копия AM кривой
	std::vector<double> pm_original = pm_values;  // Копия PM кривой
	std::vector<double> fm_original = fm_values;  // Копия FM кривой

	// Сглаживаем каждую кривую индивидуально
	smoothSymmetricWithInitialDrop(am_values, x_axis);  // Сглаживание AM
	smoothSymmetricWithInitialDrop(pm_values, x_axis);  // Сглаживание PM
	smoothSymmetricWithInitialDrop(fm_values, x_axis);  // Сглаживание FM

	// Сохраняем относительные пропорции между кривыми
	// (чтобы PM всегда оставался выше AM и FM, если это было в исходных данных)
	for (size_t i = 0; i < am_values.size(); i++) {
		// Вычисляем исходные отношения
		double total_original = am_original[i] + pm_original[i] + fm_original[i];  // Сумма исходных значений
		if (total_original > 0) {
			double am_ratio = am_original[i] / total_original;  // Доля AM в общей сумме
			double pm_ratio = pm_original[i] / total_original;  // Доля PM в общей сумме
			double fm_ratio = fm_original[i] / total_original;  // Доля FM в общей сумме

			// Новая общая сумма после сглаживания
			double total_smoothed = am_values[i] + pm_values[i] + fm_values[i];

			// Корректируем с сохранением пропорций (30% от пропорций, 70% от сглаженных)
			am_values[i] = total_smoothed * am_ratio * 0.3 + am_values[i] * 0.7;
			pm_values[i] = total_smoothed * pm_ratio * 0.3 + pm_values[i] * 0.7;
			fm_values[i] = total_smoothed * fm_ratio * 0.3 + fm_values[i] * 0.7;
		}
	}
}

// =============== НОВАЯ ФУНКЦИЯ mainProcessNew ===============
// Функция для обработки сигнала и получения всех необходимых данных для отображения
ProcessResult mainProcessNew(double _fd, int _nbits, double _bitrate, double _fc,
	double _delay, double _snr, double _snr_fully,
	double duration_base, type_modulation _type)
{
	ProcessResult result;  // Структура для хранения результатов

	modulation manipulated;  // Объект модуляции
	// Установка параметров модуляции
	manipulated.setParam(_fd, _nbits, _bitrate, _fc, _delay, duration_base, _type);
	manipulated.manipulation();  // Выполнение модуляции
	auto base_signal = manipulated.createBaseSignal();  // Создание базового сигнала
	auto fully_signal = manipulated.getS();  // Получение полного сигнала
	auto t = manipulated.getT();  // Получение временной шкалы

	signal base_signal_noise;  // Базовый сигнал с шумом
	signal fully_signal_noise;  // Полный сигнал с шумом
	// Добавление шума к сигналам
	addNoise(base_signal, _snr, base_signal_noise);  // Шум для базового сигнала
	addNoise(fully_signal, _snr_fully, fully_signal_noise);  // Шум для полного сигнала

	std::vector<double> corr;  // Вектор корреляции
	std::vector<double> t_corr;  // Вектор времени для корреляции
	// Вычисление корреляции между сигналами
	correlation(fully_signal_noise, base_signal_noise, 1. / (_fd * 1000), corr, t_corr);

	// Сохраняем обе компоненты сигнала в структуру результата
	result.bitsY = manipulated.getBits();  // Битовая последовательность (Y)
	result.bitsX = t;  // Временная шкала для битов (X)
	result.fullSignalInphaseY = fully_signal_noise.getSreal();  // Синфазная компонента (Y)
	result.fullSignalQuadratureY = fully_signal_noise.getSimag(); // Квадратурная компонента (Y)
	result.fullSignalX = t;  // Временная шкала для сигнала (X)
	result.correlationY = corr;  // Корреляционная функция (Y)
	result.correlationX = t_corr;  // Временная шкала для корреляции (X)
	result.signalDuration = manipulated.getDuration();  // Длительность сигнала

	// Поиск максимальной корреляции и оценка задержки
	result.delayEstimation = findMax(corr, t_corr, _delay);

	return result;  // Возврат структуры с результатами
}

// =============== СТАРАЯ ФУНКЦИЯ (для совместимости) ===============
// Оригинальная функция обработки сигнала (возвращает только задержку и критерий)
std::pair<double, double> mainProcessOld(double _fd, int _nbits, double _bitrate,
	double _fc, double _delay, double _snr,
	double _snr_fully, double duration_base,
	type_modulation _type)
{
	// Оригинальный код остается без изменений
	modulation manipulated;  // Объект модуляции
	// Установка параметров
	manipulated.setParam(_fd, _nbits, _bitrate, _fc, _delay, duration_base, _type);
	manipulated.manipulation();  // Модуляция
	auto base_signal = manipulated.createBaseSignal();  // Базовый сигнал
	auto fully_signal = manipulated.getS();  // Полный сигнал
	auto t = manipulated.getT();  // Временная шкала

	signal base_signal_noise;  // Базовый сигнал с шумом
	signal fully_signal_noise;  // Полный сигнал с шумом
	// Добавление шума
	addNoise(base_signal, _snr, base_signal_noise);  // Шум для базового сигнала
	addNoise(fully_signal, _snr_fully, fully_signal_noise);  // Шум для полного сигнала

	std::vector<double> corr;  // Корреляция
	std::vector<double> t_corr;  // Время для корреляции
	// Вычисление корреляции
	correlation(fully_signal_noise, base_signal_noise, 1. / (_fd * 1000), corr, t_corr);

	int sample = fully_signal_noise.N;  // Размер выборки
	// Изменение размера сигналов (избыточная операция)
	fully_signal_noise.resize(sample);
	base_signal_noise.resize(sample);

	// Ваш оригинальный код с criteria
	auto a = findMax(corr, t_corr, _delay);  // Поиск максимума корреляции
	a.second = criteria(corr);  // Важно: используем criteria как в оригинале

	return a;  // Возврат пары (задержка, критерий)
}

// Обработчик нажатия кнопки "Создать"
void Cambiguity_functionDlg::OnBnClickedButcreate()
{
	UpdateData(TRUE);  // Обновление данных из элементов управления

	// Вызов новой функции обработки сигнала
	ProcessResult result = mainProcessNew(fd, nbits, bitrate, fc, delay,
		snr, snr_fully, sample_base, type);

	// Сохраняем данные для отрисовки
	m_bitsY = result.bitsY;  // Биты (Y)
	m_bitsX = result.bitsX;  // Время для битов (X)
	m_fullSignalInphaseY = result.fullSignalInphaseY;      // Синфазная компонента (Y)
	m_fullSignalQuadratureY = result.fullSignalQuadratureY; // Квадратурная компонента (Y)

	// ИСПРАВЛЕНИЕ: создаем новый временной вектор для отображения
	// Используем количество точек сигнала и сигнальную длительность
	double signalDuration = result.signalDuration;  // Длительность сигнала
	int sampleCount = m_fullSignalInphaseY.size();  // Количество отсчетов

	m_fullSignalX.clear();  // Очистка предыдущего вектора
	m_fullSignalX.resize(sampleCount);  // Изменение размера
	// Создание равномерной временной шкалы
	for (int i = 0; i < sampleCount; i++) {
		m_fullSignalX[i] = signalDuration * i / (sampleCount - 1);  // Линейное распределение времени
	}

	m_correlationY = result.correlationY;  // Корреляция (Y)
	m_correlationX = result.correlationX;  // Время для корреляции (X)

	// 1. Кодовая последовательность (IDC_GRAPH5)
	if (!m_bitsY.empty() && !m_bitsX.empty()) {
		double minY = 0.0;  // Минимальное значение Y для битов
		double maxY = 1.5;  // Максимальное значение Y для битов
		double minX = 0.0;  // Минимальное время
		double maxX = signalDuration;  // Максимальное время (длительность сигнала)
		// Отрисовка битовой последовательности
		drv5.Draw(m_bitsY, minY, maxY, m_bitsX, minX, maxX, 'B', L"t, с", L"Бит");
	}

	// 2. Полный сигнал - ДВЕ КОМПОНЕНТЫ (IDC_GRAPH2)
	if (!m_fullSignalInphaseY.empty() && !m_fullSignalX.empty() &&
		!m_fullSignalQuadratureY.empty()) {

		// Находим общие min/max для обеих компонент
		double minInphase = *std::min_element(m_fullSignalInphaseY.begin(),
			m_fullSignalInphaseY.end());  // Минимум синфазной компоненты
		double maxInphase = *std::max_element(m_fullSignalInphaseY.begin(),
			m_fullSignalInphaseY.end());  // Максимум синфазной компоненты
		double minQuad = *std::min_element(m_fullSignalQuadratureY.begin(),
			m_fullSignalQuadratureY.end());  // Минимум квадратурной компоненты
		double maxQuad = *std::max_element(m_fullSignalQuadratureY.begin(),
			m_fullSignalQuadratureY.end());  // Максимум квадратурной компоненты

		double minY = std::min(minInphase, minQuad);  // Общий минимум
		double maxY = std::max(maxInphase, maxQuad);  // Общий максимум

		// Добавляем 10% запаса по вертикали
		double range = maxY - minY;  // Диапазон значений
		if (range < 0.001) range = 2.0; // защита от нулевого диапазона
		minY -= range * 0.1;  // Расширение снизу
		maxY += range * 0.1;  // Расширение сверху

		// Временной диапазон строго от 0 до signalDuration
		double minX = 0.0;  // Минимальное время
		double maxX = signalDuration;  // Максимальное время

		// Используем Draw2 для двух компонент с одинаковыми временными метками
		drv2.Draw2(m_fullSignalInphaseY, minY, maxY,
			m_fullSignalX, minX, maxX, 'R',  // Синфазная - красный
			m_fullSignalQuadratureY, m_fullSignalX, 'G',  // Квадратурная - зеленый
			L"t, с", L"Амплитуда");
	}

	// 3. Корреляция (IDC_GRAPH1)
	if (!m_correlationY.empty() && !m_correlationX.empty()) {
		double minY = *std::min_element(m_correlationY.begin(), m_correlationY.end());  // Минимум корреляции
		double maxY = *std::max_element(m_correlationY.begin(), m_correlationY.end());  // Максимум корреляции
		double minX = *std::min_element(m_correlationX.begin(), m_correlationX.end());  // Минимум времени
		double maxX = *std::max_element(m_correlationX.begin(), m_correlationX.end());  // Максимум времени

		// Добавим небольшой запас
		double yRange = maxY - minY;  // Диапазон по Y
		if (yRange > 0) {
			minY -= yRange * 0.05;  // Запас снизу
			maxY += yRange * 0.05;  // Запас сверху
		}

		// Отрисовка корреляционной функции
		drv1.Draw(m_correlationY, minY, maxY, m_correlationX, minX, maxX,
			'G', L"τ, с", L"R(τ)");  // Зеленый цвет
	}

	// Отображаем результат оценки задержки через старую функцию (для получения критерия)
	auto resultWithCriteria = mainProcessOld(fd, nbits, bitrate, fc, delay,
		snr, snr_fully, sample_base, type);

	// Формирование строки результата
	if (abs(resultWithCriteria.first - delay / 1000) > 0.5 / bitrate) {
		// Некорректный результат (больше одного бита)
		result_delay.Format(_T("Оцененный сдвиг: %.4f мс;\nКритерий выраженности: %.3f %%;\n"
			"Некорректно: больше, чем один бит"),
			resultWithCriteria.first * 1000, resultWithCriteria.second);
	}
	else {
		// Корректный результат (в пределах одного символа)
		result_delay.Format(_T("Оцененный сдвиг: %.4f мс;\nКритерий выраженности: %.3f %%;\n"
			"Корректно: в пределах одного символа"),
			resultWithCriteria.first * 1000, resultWithCriteria.second);
	}

	UpdateData(FALSE);  // Обновление элементов управления данными
}

// =============== Остальные функции ===============
// Обработчики выбора типа модуляции
void Cambiguity_functionDlg::OnBnClickedRadioAm() { type = AM; }  // АМ модуляция
void Cambiguity_functionDlg::OnBnClickedRadioPm() { type = PM; }  // ФМ модуляция
void Cambiguity_functionDlg::OnBnClickedRadioFrm() { type = FM; }  // ЧМ модуляция

// mainThread - НЕ МЕНЯЕМ
// Функция для многопоточного расчета критерия выраженности
int mainThread(double _fd, int _nbits, double _bitrate, double _fc, double _delay,
	double _snr, double _snr_fully, double duration_base,
	type_modulation _type, double& result, int N_generate)
{
	// Ваш оригинальный код с criteria - не меняем
	int N_exp = N_generate;  // Количество экспериментов для усреднения
	result = 0;  // Инициализация результата
	for (int k = 0; k < N_exp; k++) {
		modulation manipulated;  // Объект модуляции
		// Установка параметров
		manipulated.setParam(_fd, _nbits, _bitrate, _fc, _delay, duration_base, _type);
		manipulated.manipulation();  // Модуляция
		auto base_signal = manipulated.createBaseSignal();  // Базовый сигнал
		auto fully_signal = manipulated.getS();  // Полный сигнал

		signal base_signal_noise;  // Базовый сигнал с шумом
		signal fully_signal_noise;  // Полный сигнал с шумом
		// Добавление шума
		addNoise(base_signal, _snr, base_signal_noise);  // Шум для базового сигнала
		addNoise(fully_signal, _snr_fully, fully_signal_noise);  // Шум для полного сигнала

		std::vector<double> corr;  // Корреляция
		std::vector<double> t_corr;  // Время для корреляции
		// Вычисление корреляции
		correlation(fully_signal_noise, base_signal_noise, 1. / (_fd * 1000), corr, t_corr);
		result += criteria(corr);  // Ваш criteria остается
	}
	result /= N_exp;  // Усреднение по всем экспериментам
	return 0;  // Успешное завершение
}

// Функция запуска потока для экспериментов
void StartThread(Cambiguity_functionDlg* dlg)
{
	double min_snr = dlg->min_snr;  // Минимальное ОСШ
	double max_snr = dlg->max_snr;  // Максимальное ОСШ
	double step_snr;  // Шаг изменения ОСШ
	// Логарифмическое преобразование шага для равномерного распределения на логарифмической шкале
	if (dlg->step_snr >= 2) step_snr = log(dlg->step_snr);
	else step_snr = dlg->step_snr;
	if (dlg->step_snr >= 8) step_snr = log(log(dlg->step_snr));
	int num_thread = 6;  // Количество потоков (2 на каждый тип модуляции)
	int point = (max_snr - min_snr) / step_snr;  // Количество точек для экспериментов

	// Инициализируем векторы для хранения результатов
	dlg->vec_p[0].clear();  // Очистка вектора для AM
	dlg->vec_p[1].clear();  // Очистка вектора для PM
	dlg->vec_p[2].clear();  // Очистка вектора для FM
	dlg->vec_snr.clear();   // Очистка вектора ОСШ

	dlg->vec_p[0].resize(point);  // Изменение размера вектора AM
	dlg->vec_p[1].resize(point);  // Изменение размера вектора PM
	dlg->vec_p[2].resize(point);  // Изменение размера вектора FM
	dlg->vec_snr.resize(point);   // Изменение размера вектора ОСШ

	// Сохраняем SNR значения для отрисовки
	dlg->m_experimentSNR_X.clear();  // Очистка вектора X для экспериментов
	dlg->m_experimentSNR_X.resize(point);  // Изменение размера
	for (int i = 0; i < point; i++) {
		dlg->m_experimentSNR_X[i] = min_snr + i * step_snr;  // Линейное распределение ОСШ
		dlg->vec_snr[i] = dlg->m_experimentSNR_X[i]; // Дублируем для обратной совместимости
	}

	std::vector<std::thread> thr(num_thread);  // Вектор потоков
	std::vector<type_modulation> types({ AM, PM, FM, AM, PM, FM });  // Типы модуляции для потоков

	// Основной цикл обработки
	for (int i = 0; i < point; i += 2) {
		// Обновляем прогресс-бар
		int progress = static_cast<int>((static_cast<double>(i) / point) * 100);  // Расчет прогресса
		dlg->progress_experement.SetPos(progress);  // Установка позиции прогресс-бара

		// Обновляем отображение в основном потоке
		if (dlg->GetSafeHwnd()) {
			// Форматирование строки прогресса
			dlg->result_delay.Format(_T("Выполняется расчет... %d%%"),
				//(%d/%d)"),
				progress, i, point);
			dlg->UpdateData(FALSE);  // Обновление интерфейса
		}

		// Запускаем потоки для текущей пары точек
		for (int j = 0; j < num_thread; j++) {
			if (i + j / 3 >= point) break;  // Проверка выхода за границы

			// Ждем завершения предыдущего потока, если он еще работает
			if (thr[j].joinable()) {
				thr[j].join();  // Ожидание завершения потока
			}

			// Запуск потока для расчета критерия
			thr[j] = std::thread(
				mainThread,  // Функция для выполнения в потоке
				dlg->fd,  // Частота дискретизации
				dlg->nbits,  // Количество бит
				dlg->bitrate,  // Битрейт
				dlg->vec_snr[i + j / 3],  // ОСШ (используется как несущая частота)
				dlg->delay,  // Задержка
				dlg->snr,      // SNR для базового сигнала
				dlg->snr_fully, // SNR для полного сигнала
				dlg->sample_base,  // Длительность базового сигнала
				types[j],  // Тип модуляции
				std::ref(dlg->vec_p[types[j]][i + j / 3]),  // Ссылка на результат
				dlg->N_generate);  // Количество генераций
		}

		// Ждем завершения всех потоков перед следующей итерацией
		for (int j = 0; j < num_thread; j++) {
			if (thr[j].joinable()) {
				thr[j].join();  // Ожидание завершения
			}
		}
	}

	// Устанавливаем прогресс в 100%
	dlg->progress_experement.SetPos(100);

	// Сохраняем результаты экспериментов
	dlg->m_experimentAM_Y = dlg->vec_p[0];  // Результаты для AM
	dlg->m_experimentPM_Y = dlg->vec_p[1];  // Результаты для PM
	dlg->m_experimentFM_Y = dlg->vec_p[2];  // Результаты для FM

	// Сглаживание кривых экспериментов
	smoothAllCurvesSymmetric(dlg->m_experimentAM_Y,
		dlg->m_experimentPM_Y,
		dlg->m_experimentFM_Y,
		dlg->m_experimentSNR_X);

	// Находим диапазоны для осей
	double minY = 0.0; // Критерий выраженности обычно от 0
	double maxY = 0.0; // Максимальное значение по Y

	// Находим общий максимум по всем трем кривым
	double maxAM = *std::max_element(dlg->m_experimentAM_Y.begin(), dlg->m_experimentAM_Y.end());  // Максимум AM
	double maxPM = *std::max_element(dlg->m_experimentPM_Y.begin(), dlg->m_experimentPM_Y.end());  // Максимум PM
	double maxFM = *std::max_element(dlg->m_experimentFM_Y.begin(), dlg->m_experimentFM_Y.end());  // Максимум FM
	maxY = std::max({ maxAM, maxPM, maxFM });  // Общий максимум

	// Добавляем 10% запаса сверху
	maxY += maxY * 0.1;

	// Находим min/max для оси X
	double minX = *std::min_element(dlg->m_experimentSNR_X.begin(),
		dlg->m_experimentSNR_X.end());  // Минимум по X
	double maxX = *std::max_element(dlg->m_experimentSNR_X.begin(),
		dlg->m_experimentSNR_X.end());  // Максимум по X

	// Добавляем небольшой запас по краям
	double xRange = maxX - minX;  // Диапазон по X
	if (xRange > 0) {
		minX -= xRange * 0.01;  // Запас слева
		maxX += xRange * 0.01;  // Запас справа
	}
	else {
		// Если все значения одинаковые, создаем диапазон
		minX -= 1.0;
		maxX += 1.0;
	}

	// Проверяем, что размеры векторов совпадают
	if (dlg->m_experimentAM_Y.size() == dlg->m_experimentSNR_X.size() &&
		dlg->m_experimentPM_Y.size() == dlg->m_experimentSNR_X.size() &&
		dlg->m_experimentFM_Y.size() == dlg->m_experimentSNR_X.size()) {

		// Используем Draw3() для отрисовки всех трех кривых
		dlg->drv4.Draw3(
			dlg->m_experimentAM_Y, minY, maxY,  // Данные AM
			dlg->m_experimentSNR_X, minX, maxX, 'R',  // Ось X, AM - красный
			dlg->m_experimentPM_Y,  // Данные PM
			dlg->m_experimentSNR_X,  // Для PM используем те же X
			'G',  // PM - зеленый
			dlg->m_experimentFM_Y,  // Данные FM
			dlg->m_experimentSNR_X,  // Для FM используем те же X
			'B',  // FM - синий
			L"SNR, дБ",  // Подпись оси X
			L"Критерий выраженности"  // Подпись оси Y
		);
	}
}

// Обработчик нажатия кнопки "Оценить" (запуск экспериментов)
void Cambiguity_functionDlg::OnBnClickedButtonEstimate()
{
	stepff = step_snr;  // Сохранение шага для сглаживания
	UpdateData();  // Обновление данных из интерфейса
	std::thread t1(StartThread, this);  // Запуск потока для экспериментов
	t1.detach();  // Отсоединение потока (не ждем завершения)
}

// mainSecondProcess с параметрами для возврата данных функции неопределенности
std::pair<double, double> mainSecondProcess(double _fd, int _nbits, double _bitrate,
	double _fc, double _delay, double _snr,
	double _snr_fully, double duration_base,
	type_modulation _type,
	std::vector<std::vector<double>>& ambiguityData,
	std::vector<double>& ambiguityFreq,
	std::vector<double>& ambiguityTime)
{
	modulation manipulated;  // Объект модуляции
	// Установка параметров
	manipulated.setParam(_fd, _nbits, _bitrate, _fc, _delay, duration_base, _type);
	manipulated.manipulation();  // Модуляция
	auto base_signal = manipulated.createBaseSignal();  // Базовый сигнал
	auto fully_signal = manipulated.getS();  // Полный сигнал
	auto t = manipulated.getT();  // Временная шкала

	signal base_signal_noise;  // Базовый сигнал с шумом
	signal fully_signal_noise;  // Полный сигнал с шумом
	// Добавление шума
	addNoise(base_signal, _snr, base_signal_noise);  // Шум для базового сигнала
	addNoise(fully_signal, _snr_fully, fully_signal_noise);  // Шум для полного сигнала

	std::vector<double> ff;  // Частотная ось
	std::vector<double> tau;  // Временная ось
	// Создание взаимной функции неопределенности
	auto image = create_f_t(fully_signal_noise, base_signal_noise, _fd * 1000, ff, tau);

	// Сохраняем данные для отрисовки
	ambiguityData = image;  // Данные функции неопределенности
	ambiguityFreq = ff;     // Частотная ось
	ambiguityTime = tau;    // Временная ось

	// Поиск сдвига по функции неопределенности
	auto a = find_sdvig(image, ff, tau);
	return a;  // Возврат пары (частота, время)
}

// Обработчик нажатия кнопки "ФН" (горячая карта)
void Cambiguity_functionDlg::OnBnClickedButton1()
{
	UpdateData(TRUE);  // Обновление данных из интерфейса

	std::vector<std::vector<double>> ambiguityData;  // Данные ФН
	std::vector<double> ambiguityFreq;  // Частотная ось
	std::vector<double> ambiguityTime;  // Временная ось

	// Расчет функции неопределенности
	auto mark_delay = mainSecondProcess(fd, nbits, bitrate, -fc, delay,
		snr, snr_fully, sample_base, type,
		ambiguityData, ambiguityFreq, ambiguityTime);

	// Сохраняем данные ФН
	m_ambiguityData = ambiguityData;  // Данные
	m_ambiguityFreq = ambiguityFreq;  // Частота
	m_ambiguityTime = ambiguityTime;  // Время

	// Задаем диапазон отображения
	drv4.SetViewRange(m_customFreqMin, m_customFreqMax, m_customTimeMin, m_customTimeMax);

	// ОЧЕНЬ ВАЖНО: mark_delay.second - это время в секундах
	// Если он возвращается в мс, нужно преобразовать:
	// double time_sec = mark_delay.second / 1000.0;
	// Но скорее всего из mainSecondProcess он возвращается в секундах

	// Отрисовываем ФН
	if (!m_ambiguityData.empty() && !m_ambiguityFreq.empty() && !m_ambiguityTime.empty()) {
		// ПЕРЕДАЕМ mark_delay.second КАК ЕСТЬ (предположительно в секундах)
		drv4.DrawAmbiguityFunction(
			m_ambiguityData,  // Данные
			m_ambiguityFreq,  // Частотная ось
			m_ambiguityTime,  // Временная ось
			L"Функция неопределенности",  // Заголовок
			L"Δf, Гц",  // Подпись оси X (разность частот)
			L"Δt, мс",  // Подпись оси Y (разность времени)
			true,  // Флаг отображения сетки
			mark_delay.first,       // частота в Гц
			mark_delay.second       // время в секундах
		);
	}

	// Отображаем результат
	result_delay.Format(_T("Оцененное доплеровское смещение: %.4f Гц;\n"
		"Оцененный сдвиг: %.4f мс"),
		mark_delay.first, mark_delay.second * 1000.0);  // *1000 для отображения в мс

	UpdateData(FALSE);  // Обновление интерфейса
}

// Обработчик нажатия кнопки "ФН" (контурная карта)
void Cambiguity_functionDlg::OnBnClickedButton3()
{
	UpdateData(TRUE);  // Обновление данных из интерфейса

	std::vector<std::vector<double>> ambiguityData;  // Данные ФН
	std::vector<double> ambiguityFreq;  // Частотная ось
	std::vector<double> ambiguityTime;  // Временная ось

	// Расчет функции неопределенности
	auto mark_delay = mainSecondProcess(fd, nbits, bitrate, -fc, delay,
		snr, snr_fully, sample_base, type,
		ambiguityData, ambiguityFreq, ambiguityTime);

	// Сохраняем данные ФН
	m_ambiguityData = ambiguityData;  // Данные
	m_ambiguityFreq = ambiguityFreq;  // Частота
	m_ambiguityTime = ambiguityTime;  // Время

	// Задаем диапазон отображения
	drv4.SetViewRange(m_customFreqMin, m_customFreqMax, m_customTimeMin, m_customTimeMax);

	// ОЧЕНЬ ВАЖНО: mark_delay.second - это время в секундах
	// Если он возвращается в мс, нужно преобразовать:
	// double time_sec = mark_delay.second / 1000.0;
	// Но скорее всего из mainSecondProcess он возвращается в секундах

	// Отрисовываем ФН в виде контурной карты
	if (!m_ambiguityData.empty() && !m_ambiguityFreq.empty() && !m_ambiguityTime.empty()) {
		// ПЕРЕДАЕМ mark_delay.second КАК ЕСТЬ (предположительно в секундах)
		drv4.DrawAmbiguityFunctionContour(
			m_ambiguityData,  // Данные
			m_ambiguityFreq,  // Частотная ось
			m_ambiguityTime,  // Временная ось
			L"Функция неопределенности",  // Заголовок
			L"Δf, Гц",  // Подпись оси X
			L"Δt, мс",  // Подпись оси Y
			true,  // Флаг отображения сетки
			mark_delay.first,  // Частота в Гц
			mark_delay.second,  // Время в секундах
			m_customLevels,      // Количество уровней изолиний (из диалога)
			true    // инвертировать цвета
		);
	}

	// Отображаем результат
	result_delay.Format(_T("Оцененное доплеровское смещение: %.4f Гц;\n"
		"Оцененный сдвиг: %.4f мс"),
		mark_delay.first, mark_delay.second * 1000.0);  // *1000 для отображения в мс

	UpdateData(FALSE);  // Обновление интерфейса
}