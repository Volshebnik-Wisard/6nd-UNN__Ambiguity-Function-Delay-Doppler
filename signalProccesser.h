#pragma once
#include <vector>
#include <fstream>
#include <cmath>
#include <complex>
#include <algorithm>
#include <random>
#include <numeric>
#include <thread>

typedef std::complex<double> base; // Определение типа base как комплексного числа double

// Перечисление типов модуляции
enum type_modulation {
	AM, // Амплитудная модуляция
	PM, // Фазовая модуляция
	FM  // Частотная модуляция
};

// Структура для представления сигнала
struct signal {
	int N; // Количество отсчетов
	std::vector<double> I; // Синфазная компонента (действительная часть)
	std::vector<double> Q; // Квадратурная компонента (мнимая часть)
	std::vector<base> s; // Комплексный сигнал

	void resize(int n) { // Метод изменения размера сигнала
		I.resize(n); // Изменение размера синфазной компоненты
		Q.resize(n); // Изменение размера квадратурной компоненты
		s.resize(n); // Изменение размера комплексного сигнала
	}

	void erase(int delay, int sample) { // Метод вырезания фрагмента сигнала
		I = std::vector<double>(I.begin() + delay, I.begin() + delay + sample); // Вырезание синфазной компоненты
		Q = std::vector<double>(Q.begin() + delay, Q.begin() + delay + sample); // Вырезание квадратурной компоненты
		s = std::vector<base>(s.begin() + delay, s.begin() + delay + sample); // Вырезание комплексного сигнала
	}

	int size() { // Метод получения размера сигнала
		return I.size(); // Возврат размера синфазной компоненты
	}

	std::vector<double> getSreal() { // Метод получения действительной части сигнала
		std::vector<double> res(s.size()); // Вектор для результата
		for (int i = 0; i < res.size(); i++) { // Цикл по всем элементам
			res[i] = s[i].real(); // Получение действительной части
		}
		return res; // Возврат результата
	}

	std::vector<double> getSimag() { // Метод получения мнимой части сигнала
		std::vector<double> res(s.size()); // Вектор для результата
		for (int i = 0; i < res.size(); i++) { // Цикл по всем элементам
			res[i] = s[i].imag(); // Получение мнимой части
		}
		return res; // Возврат результата
	}
};

// Объявление функции генерации битовой последовательности
std::vector<bool> generate_bits(int nbits, int sample);

// Объявление функции быстрого преобразования Фурье
void fft(std::vector<base>& a, bool invert);

// Объявление функции вычисления корреляции двух сигналов
void correlation(signal s1, signal s2, double step, std::vector<double>& corr, std::vector<double>& t);

// Объявление функции вычисления корреляции методом суммирования
std::pair<std::vector<double>, std::vector<double>> correlationSumma(signal s1, signal s2);

// Объявление функции нормализации размеров сигналов
void normalizeSize(signal& s1, signal& s2);

// Объявление функции поиска максимума корреляции
std::pair<double, double> findMax(std::vector<double> corr, std::vector<double> t, double delay);

// Объявление функции добавления шума к сигналу
void addNoise(signal s, double snr, signal& s_n);

// Объявление функции вычисления критерия качества корреляции
double criteria(std::vector<double> corr);

// Объявление функции вычисления одного сдвига для взаимной функции неопределенности
void one_sdvig(signal s_fully, signal s, int delay, std::vector<double>& res_fft);

// Объявление функции создания взаимной функции неопределенности
std::vector<std::vector<double>> create_f_t(signal s_fully, signal s, double fd, std::vector<double>& ff, std::vector<double>& tau);

// Объявление функции поиска сдвига по взаимной функции неопределенности
std::pair<double, double> find_sdvig(std::vector<std::vector<double>>& image, std::vector<double>& ff, std::vector<double>& tau);

// Объявление класса модуляции
class modulation {
	std::vector<double> t; // Вектор временных меток
	signal s; // Сигнал
	std::vector<bool> bits; // Битовая последовательность
	double fd; // Частота дискретизации
	int nbits; // Количество бит
	int sample_base; // Длительность базового сигнала в отсчетах
	double bitrate; // Битрейт
	double deltaF; // Девиация частоты (для FM) или индекс модуляции
	int delay; // Задержка в отсчетах
	double snr; // Отношение сигнал/шум
	type_modulation type; // Тип модуляции
	double duration; // Длительность сигнала в секундах
	int sample; // Количество отсчетов сигнала

public:
	// Объявление методов класса
	void setParam(double _fd, int _nbits, double _bitrate, double _fc, double _delay, double _sample_base, type_modulation _type);

	void generate_t();

	void modAM();

	void modPM2();

	void modFM2();

	signal getS();

	std::vector<double> getT();

	double getDuration();

	std::vector<double> getBits();

	void manipulation();

	void setType(type_modulation _type);

	signal createBaseSignal();
};