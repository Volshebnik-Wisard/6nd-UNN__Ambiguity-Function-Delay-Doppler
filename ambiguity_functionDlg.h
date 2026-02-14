// Защита от многократного включения заголовочного файла
#pragma once

// Включаем необходимые заголовочные файлы
#include "signalProccesser.h"  // Для обработки сигналов
#include "Drawer.h"            // Для отрисовки графиков

// Добавляем структуру как в olddelayDlg.cpp
// Структура для хранения результатов обработки сигнала
struct ProcessResult {
	std::pair<double, double> delayEstimation;         // Пара (оцененная задержка, ошибка)
	std::vector<double> bitsY;                         // Битовая последовательность (ось Y)
	std::vector<double> bitsX;                         // Временная ось для битов (ось X)
	std::vector<double> fullSignalInphaseY;            // Синфазная компонента полного сигнала (Y)
	std::vector<double> fullSignalQuadratureY;         // Квадратурная компонента полного сигнала (Y)
	std::vector<double> fullSignalX;                   // Временная ось для полного сигнала (X)
	std::vector<double> correlationY;                  // Корреляционная функция (Y)
	std::vector<double> correlationX;                  // Временная ось для корреляции (X)
	double signalDuration;                             // Длительность сигнала в секундах
};

// Класс диалогового окна для работы с функцией неопределенности
class Cambiguity_functionDlg : public CDialogEx  // Наследование от базового класса диалогов MFC
{
public:
	// Конструктор класса
	Cambiguity_functionDlg(CWnd* pParent = nullptr);  // Параметр - родительское окно (по умолчанию нет)

	// Макросы для визуального проектирования в MFC
#ifdef AFX_DESIGN_TIME
	enum { IDD = IDD_CHECK_DELAY_DIALOG };  // Идентификатор ресурса диалогового окна
#endif

protected:
	// Виртуальная функция обмена данными между элементами управления и переменными
	virtual void DoDataExchange(CDataExchange* pDX);

	HICON m_hIcon;  // Дескриптор иконки приложения

	// Виртуальная функция инициализации диалога
	virtual BOOL OnInitDialog();

	// Обработчик сообщения WM_PAINT (перерисовка окна)
	afx_msg void OnPaint();

	// Обработчик запроса курсора для перетаскивания окна
	afx_msg HCURSOR OnQueryDragIcon();

	// Макрос объявления карты сообщений MFC
	DECLARE_MESSAGE_MAP()

public:
	// Данные для графиков - ТОЧНО КАК В olddelayDlg

	// Данные для графика полного сигнала (синфазная и квадратурная компоненты)
	std::vector<double> m_fullSignalInphaseY;     // Синфазная компонента (действительная часть)
	std::vector<double> m_fullSignalQuadratureY;  // Квадратурная компонента (мнимая часть)
	std::vector<double> m_fullSignalX;            // Временная ось для полного сигнала

	// Данные для графика корреляционной функции
	std::vector<double> m_correlationY;           // Значения корреляционной функции
	std::vector<double> m_correlationX;           // Временная ось для корреляции

	// Данные для графика кодовой последовательности
	std::vector<double> m_bitsY;                  // Значения битов (0 или 1)
	std::vector<double> m_bitsX;                  // Временная ось для битов

	// Для экспериментов (зависимость критерия от SNR для разных типов модуляции)
	std::vector<double> m_experimentAM_Y;         // Значения критерия для AM модуляции
	std::vector<double> m_experimentPM_Y;         // Значения критерия для PM модуляции
	std::vector<double> m_experimentFM_Y;         // Значения критерия для FM модуляции
	std::vector<double> m_experimentSNR_X;        // Значения SNR для оси X

	// Данные для функции неопределенности
	std::vector<std::vector<double>> m_ambiguityData;  // 2D матрица значений функции неопределенности
	std::vector<double> m_ambiguityFreq;               // Частотная ось для ФН
	std::vector<double> m_ambiguityTime;               // Временная ось для ФН

	// Объекты Drawer для отрисовки графиков на разных контролах
	Drawer drv1;  // Для графика корреляции (IDC_GRAPH1)
	Drawer drv2;  // Для графика полного сигнала (IDC_GRAPH2)
	Drawer drv3;  // Добавлено для IDC_GRAPH3 если нужно (резерв)
	Drawer drv4;  // Для графика экспериментов/ФН (IDC_GRAPH4)
	Drawer drv5;  // Для графика кодовой последовательности (IDC_GRAPH5)

	// Обработчики сообщений (объявляются как afx_msg для MFC)
	afx_msg void OnBnClickedButcreate();      // Обработчик кнопки "Создать"

	// Параметры сигнала (привязаны к элементам управления через DoDataExchange)
	double fd;          // Частота дискретизации (в МГц)
	int nbits;          // Количество битов
	double bitrate;     // Битрейт (в бит/с)
	double fc;          // Несущая частота (в Гц)
	double delay;       // Задержка (в мс)
	double snr;         // Отношение сигнал/шум для базового сигнала (в дБ)
	modulation modul_signal;  // Объект для работы с модуляцией
	CString result_delay;     // Строка для отображения результата оценки задержки
	type_modulation type;     // Тип модуляции (AM, PM, FM)

	afx_msg void OnBnClickedRadioAm();   // Обработчик радиокнопки AM
	afx_msg void OnBnClickedRadioPm();   // Обработчик радиокнопки PM
	afx_msg void OnBnClickedRadioFrm();  // Обработчик радиокнопки FM

	double sample_base;  // Длительность базового сигнала (в отсчетах)
	double snr_fully;    // ОСШ для полного сигнала (в дБ)

	// Векторы для хранения результатов экспериментов
	std::vector<std::vector<double>> vec_p = std::vector<std::vector<double>>(3);  // 3 вектора для AM, PM, FM
	std::vector<double> vec_snr;  // Вектор значений SNR для экспериментов

	afx_msg void OnBnClickedButtonEstimate();  // Обработчик кнопки "Оценить" (эксперименты)

	// Параметры для экспериментов
	double min_snr;      // Минимальное значение SNR (в дБ)
	double max_snr;      // Максимальное значение SNR (в дБ)
	double step_snr;     // Шаг изменения SNR
	double N_generate;   // Количество генераций для усреднения

	CProgressCtrl progress_experement;  // Элемент управления прогресс-бар

	afx_msg void OnBnClickedButton1();  // Обработчик кнопки "ФН" (горячая карта)
	afx_msg void OnBnClickedButton3();  // Обработчик кнопки "ФН" (контурная карта)

	// Ссылки на статические элементы управления для графиков
	CStatic m_picCtrlFullSignal;     // Для полного сигнала (IDC_GRAPH2)
	CStatic m_picCtrlCorrelation;    // Для корреляции (IDC_GRAPH1)
	CStatic m_picCtrlExperiment;     // Для экспериментов (IDC_GRAPH4)
	CStatic m_picCodeSequence;       // Для кодовой последовательности (IDC_GRAPH5)

	// Параметры для кастомизации отображения функции неопределенности
	double m_customFreqMin;   // Минимальная частота для отображения
	double m_customFreqMax;   // Максимальная частота для отображения
	double m_customTimeMin;   // Минимальное время для отображения
	double m_customTimeMax;   // Максимальное время для отображения
	double m_customLevels;    // Количество уровней для контурной карты
};

// Объявления глобальных функций

// Основная функция для многопоточного расчета критерия выраженности
// Параметры:
// _fd - частота дискретизации
// _nbits - количество бит
// _bitrate - битрейт
// _fc - несущая частота
// _delay - задержка
// _snr - ОСШ для базового сигнала
// _snr_fully - ОСШ для полного сигнала
// duration_base - длительность базового сигнала
// _type - тип модуляции
// result - ссылка для возврата результата
// N_generate - количество генераций
int mainThread(double _fd, int _nbits, double _bitrate, double _fc, double _delay,
	double _snr, double _snr_fully, double duration_base,
	type_modulation _type, double& result, int N_generate);

// ДВЕ разные функции с РАЗНЫМИ именами для обработки сигнала

// Новая функция, возвращающая полные данные для отображения
ProcessResult mainProcessNew(double _fd, int _nbits, double _bitrate, double _fc,
	double _delay, double _snr, double _snr_fully,
	double duration_base, type_modulation _type);

// Старая функция (для совместимости), возвращающая только оценку задержки и критерий
std::pair<double, double> mainProcessOld(double _fd, int _nbits, double _bitrate,
	double _fc, double _delay, double _snr,
	double _snr_fully, double duration_base,
	type_modulation _type);

// Функция для расчета и возврата данных функции неопределенности
// Дополнительные параметры для возврата данных:
// ambiguityData - матрица значений ФН
// ambiguityFreq - частотная ось
// ambiguityTime - временная ось
std::pair<double, double> mainSecondProcess(double _fd, int _nbits, double _bitrate,
	double _fc, double _delay, double _snr,
	double _snr_fully, double duration_base,
	type_modulation _type,
	std::vector<std::vector<double>>& ambiguityData,
	std::vector<double>& ambiguityFreq,
	std::vector<double>& ambiguityTime);

// Функция для запуска потока экспериментов
void StartThread(Cambiguity_functionDlg* dlg);