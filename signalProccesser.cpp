#include "pch.h"
#define _USE_MATH_DEFINES
#include "signalProccesser.h"
using namespace std;

// Функция генерации битовой последовательности
std::vector<bool> generate_bits(int nbits, int sample)
{
	vector<bool> result(sample); // Вектор для результата
	vector<bool> bits(nbits); // Вектор для битов
	std::random_device rd; // Источник случайных чисел
	std::mt19937 gen(rd()); // Генератор случайных чисел
	std::bernoulli_distribution dist(0.5); // Распределение Бернулли с вероятностью 0.5

	// Генерация последовательности битов
	for (int i = 0; i < nbits; i++) {
		bits[i] = dist(gen); // Генерация бита
	}
	int sam_bit = ceil((double)sample / nbits); // Количество отсчетов на бит
	for (int i = 0; i < sample; i++) { // Цикл по всем отсчетам
		result[i] = bits[floor((double)i / sam_bit)]; // Заполнение результата
	}
	return result; // Возврат результата
}

// Реализация быстрого преобразования Фурье (БПФ)
void fft(std::vector<base>& a, bool invert)
{
	int n = (int)a.size(); // Размер массива
	if (n == 1)  return; // Базовый случай рекурсии

	vector<base> a0(n / 2), a1(n / 2); // Разделение массива на четные и нечетные элементы
	for (int i = 0, j = 0; i < n; i += 2, ++j) { // Цикл разделения
		a0[j] = a[i]; // Четные элементы
		a1[j] = a[i + 1]; // Нечетные элементы
	}
	fft(a0, invert); // Рекурсивный вызов для четных элементов
	fft(a1, invert); // Рекурсивный вызов для нечетных элементов

	double ang = 2 * M_PI / n * (invert ? -1 : 1); // Вычисление угла поворота
	base w(1), wn(cos(ang), sin(ang)); // Поворачивающий множитель
	for (int i = 0; i < n / 2; ++i) { // Комбинирование результатов
		a[i] = a0[i] + w * a1[i]; // Первая половина
		a[i + n / 2] = a0[i] - w * a1[i]; // Вторая половина
		if (invert) // Если обратное преобразование
			a[i] /= 2, a[i + n / 2] /= 2; // Деление на 2
		w *= wn; // Обновление поворачивающего множителя
	}
}

// Функция вычисления корреляции двух сигналов
void correlation(signal s1, signal s2, double step, std::vector<double>& corr, std::vector<double>& t)
{
	fft(s1.s, true); // Прямое БПФ первого сигнала
	fft(s2.s, true); // Прямое БПФ второго сигнала
	std::vector<base> result(s1.size()); // Вектор для результата
	for (int i = 0; i < result.size(); i++) { // Цикл по всем элементам
		result[i] = s1.s[i] * std::conj(s2.s[i]); // Умножение на комплексно-сопряженное
	}
	fft(result, false); // Обратное БПФ
	corr.resize(s1.N); // Изменение размера вектора корреляции
	t.resize(s1.N); // Изменение размера вектора времени
	for (int i = 0; i < corr.size(); i++) { // Цикл по всем элементам
		corr[i] = (result[i] * std::conj(result[i])).real(); // Вычисление модуля
		t[i] = i * step; // Вычисление временной метки
	}
}

// Функция вычисления корреляции методом суммирования
std::pair<std::vector<double>, std::vector<double>> correlationSumma(signal s1, signal s2)
{
	if (s1.size() > s2.size()) { // Если первый сигнал длиннее
		auto s = s1; // Обмен сигналами
		s1 = s2;
		s2 = s;
	}
	int size_shift = abs(s1.size() - s2.size()) + 1; // Количество возможных сдвигов
	std::vector<double> shift(size_shift); // Вектор сдвигов
	base temp; // Временная переменная
	std::vector<double> res(size_shift); // Вектор результатов

	int size = s1.size(); // Размер меньшего сигнала
	for (int i = 0; i < size_shift; i++) { // Цикл по всем сдвигам
		shift[i] = i; // Запись значения сдвига
		temp = base(0, 0); // Обнуление временной переменной
		for (int j = 0; j < size; j++) { // Цикл по всем отсчетам
			temp += s1.s[j] * std::conj(s2.s[i + j]); // Суммирование произведений
		}
		res[i] = temp.real() * temp.real() + temp.imag() * temp.imag(); // Вычисление квадрата модуля
	}

	return std::pair<std::vector<double>, std::vector<double>>(res, shift); // Возврат результата
}

// Функция нормализации размеров сигналов до степени двойки
void normalizeSize(signal& s1, signal& s2)
{
	int size = 0; // Переменная для максимального размера
	if (s1.size() >= s2.size()) { // Определение максимального размера
		size = s1.size();
	}
	else {
		size = s2.size();
	}
	int pw2 = 1; // Степень двойки
	while (size > pow(2, pw2)) { // Поиск ближайшей степени двойки
		pw2++;
	}
	size = pow(2, pw2); // Размер, равный степени двойки
	s1.resize(size); // Изменение размера первого сигнала
	s2.resize(size); // Изменение размера второго сигнала
}

// Функция поиска максимума корреляции
std::pair<double, double> findMax(std::vector<double> corr, std::vector<double> t, double delay)
{
	delay /= 1000; // Преобразование задержки из мс в секунды
	int indexMax = std::max_element(corr.begin(), corr.end()) - corr.begin(); // Индекс максимума
	double d_delay = 0.; // Относительная ошибка
	if (delay == 0) { // Если задержка нулевая
		if (t[indexMax] != 0) { // Если найденный максимум не в нуле
			d_delay = abs(delay - t[indexMax]) / t[indexMax]; // Вычисление относительной ошибки
		}
	}
	else {
		d_delay = abs(delay - t[indexMax]) * 100. / delay; // Вычисление процентной ошибки
	}
	return std::pair<double, double>(t[indexMax], d_delay); // Возврат результата
}

// Функция добавления шума к сигналу
void addNoise(signal s, double snr, signal& s_n)
{
	s_n = s; // Копирование сигнала
	if (s_n.s.empty()) return; // Проверка на пустоту

	std::random_device rd; // Источник случайных чисел
	std::mt19937 gen(rd()); // Генератор случайных чисел
	std::normal_distribution<double> dist(0.0, 1.0); // Нормальное распределение

	// Вычисление мощности сигнала
	double signal_power = 0.; // Переменная для мощности сигнала
	for (auto sample : s_n.s) { // Цикл по всем отсчетам
		signal_power += std::norm(sample); // Суммирование квадратов модулей
	}
	signal_power /= s_n.N; // Усреднение

	double snr_linear = std::pow(10.0, snr / 10.0); // Преобразование SNR из дБ в линейную шкалу
	double noise_power = signal_power / snr_linear; // Мощность шума
	double noise_stddev = std::sqrt(noise_power / 2.0); // Среднеквадратическое отклонение шума

	// Добавление шума
	for (int i = 0; i < s_n.N; i++) { // Цикл по всем отсчетам
		s_n.s[i] += base(dist(gen) * noise_stddev, // Добавление шума к реальной части
			dist(gen) * noise_stddev); // Добавление шума к мнимой части
	}
}

// Функция вычисления критерия качества корреляции
double criteria(std::vector<double> corr)
{
	int indexMax = std::max_element(corr.begin(), corr.end()) - corr.begin(); // Индекс максимума
	double c_max = corr[indexMax]; // Значение максимума
	double avr = 0; // Среднее значение
	for (int i = 0; i < corr.size(); i++) { // Цикл по всем значениям корреляции
		avr += corr[i]; // Суммирование
	}
	avr /= corr.size(); // Усреднение
	double sigma = 0; // Дисперсия
	for (int i = 0; i < corr.size(); i++) { // Цикл по всем значениям корреляции
		sigma += (corr[i] - avr) * (corr[i] - avr); // Суммирование квадратов отклонений
	}
	sigma = sqrt(sigma) / corr.size(); // Вычисление среднеквадратического отклонения
	return c_max / sigma; // Возврат критерия (отношение максимума к СКО)
}

// Функция вычисления одного сдвига для взаимной функции неопределенности
void one_sdvig(signal s_fully, signal s, int delay, std::vector<double>& res_fft)
{
	std::vector<base> temp; // Временный вектор
	int size_2 = s.s.size(); // Размер сигнала
	s.resize(s.N); // Изменение размера сигнала (избыточная операция)
	int size = s.N; // Размер сигнала
	temp.resize(size); // Изменение размера временного вектора
	for (int i = 0; i < size; i++) { // Цикл по всем отсчетам
		temp[i] = s.s[i] * std::conj(s_fully.s[i + delay]); // Умножение на комплексно-сопряженное со сдвигом
	}
	temp.resize(size_2); // Возврат к исходному размеру
	fft(temp, true); // Прямое БПФ
	res_fft.resize(size_2); // Изменение размера результата
	int center = size_2 / 2; // Центр массива
	for (int i = 0; i < size_2; i++) { // Цикл по всем элементам
		if (i < center) { // Если элемент в первой половине
			res_fft[i + center] = sqrt(temp[i].real() * temp[i].real() + temp[i].imag() * temp[i].imag()); // Сдвиг вправо
		}
		else { // Если элемент во второй половине
			res_fft[i - center] = sqrt(temp[i].real() * temp[i].real() + temp[i].imag() * temp[i].imag()); // Сдвиг влево
		}
	}
}

// Функция создания взаимной функции неопределенности
std::vector<std::vector<double>> create_f_t(signal s_fully, signal s, double fd, std::vector<double>& ff, std::vector<double>& tau)
{
	int max_delay = s_fully.N - s.N; // Максимальный сдвиг
	std::vector<std::vector<double>> res(max_delay); // Матрица результатов
	int max_threads = 8; // Максимальное количество потоков
	thread* thrs = new thread[max_threads]; // Массив потоков
	for (int i = 0; i < max_delay; i += max_threads) { // Цикл по сдвигам с шагом max_threads
		for (int j = 0; j < max_threads; j++) { // Цикл по потокам
			if (i + j >= max_delay) { // Проверка выхода за границы
				continue;
			}
			thrs[j] = thread(one_sdvig, // Создание потока
				s_fully,
				s,
				i + j,
				std::ref(res[i + j])
			);
		}
		for (int j = 0; j < max_threads; j++) { // Ожидание завершения потоков
			if (thrs[j].joinable()) {
				thrs[j].join();
			}
		}
	}

	ff.resize(res[0].size()); // Изменение размера вектора частот
	tau.resize(res.size()); // Изменение размера вектора временных сдвигов
	double step_ff = fd / ff.size(); // Шаг по частоте
	double step_tau = 1. / fd; // Шаг по времени
	int center = ff.size() / 2; // Центр массива частот
	for (int i = 0; i < ff.size(); i++) { // Цикл по всем частотам
		if (i < center) { // Если частота в первой половине
			ff[i + center] = i * step_ff; // Сдвиг вправо (положительные частоты)
		}
		else { // Если частота во второй половине
			ff[i - center] = -fd + i * step_ff; // Сдвиг влево (отрицательные частоты)
		}
	}
	for (int i = 0; i < tau.size(); i++) { // Цикл по всем временным сдвигам
		tau[i] = i * step_tau; // Вычисление временного сдвига
	}

	return res; // Возврат матрицы взаимной функции неопределенности
}

// Функция поиска сдвига по взаимной функции неопределенности
std::pair<double, double> find_sdvig(std::vector<std::vector<double>>& image, std::vector<double>& ff, std::vector<double>& tau)
{
	std::vector<int> max_ff(tau.size()); // Вектор индексов максимумов по частоте
	std::vector<double> max_value_ff(tau.size()); // Вектор значений максимумов
	for (int i = 0; i < image.size(); i++) { // Цикл по всем временным сдвигам
		max_ff[i] = std::max_element(image[i].begin(), image[i].end()) - image[i].begin(); // Поиск максимума по частоте
		max_value_ff[i] = image[i][max_ff[i]]; // Значение максимума
	}
	int indexMax = std::max_element(max_value_ff.begin(), max_value_ff.end()) - max_value_ff.begin(); // Поиск глобального максимума
	return std::pair<double, double>(ff[max_ff[indexMax]], tau[indexMax]); // Возврат частоты и временного сдвига
}

// Метод установки параметров модуляции
void modulation::setParam(double _fd, int _nbits, double _bitrate, double _fc, double _delay, double _sample_base, type_modulation _type)
{
	fd = _fd; // Частота дискретизации
	nbits = _nbits; // Количество бит
	bitrate = _bitrate; // Битрейт
	deltaF = _fc; // Девиация частоты (для FM) или индекс модуляции
	delay = ceil(_delay * fd); // Задержка в отсчетах
	sample_base = (double)_sample_base * fd; // Длительность базового сигнала в отсчетах
	type = _type; // Тип модуляции
	duration = nbits / bitrate; // Длительность сигнала в секундах
	if (sample != duration * fd * 1000) { // Если изменилась длительность сигнала
		sample = duration * fd * 1000; // Вычисление количества отсчетов
		generate_t(); // Генерация временных меток
		s.resize(sample); // Изменение размера сигнала
	}
}

// Метод генерации временных меток
void modulation::generate_t()
{
	t.resize(sample); // Изменение размера вектора времени
	for (int i = 0; i < sample; i++) { // Цикл по всем отсчетам
		t[i] = i / (fd * 1000); // Вычисление времени
	}
}

// Метод амплитудной модуляции (AM)
void modulation::modAM()
{
	for (int i = 0; i < sample; i++) { // Цикл по всем отсчетам
		s.I[i] = (1 + (bits[i] ? 1 : 0)) / 2.; // Амплитудная модуляция (0.5 или 1)
		//s.I[i] = bits[i] ? 1 : -1; // Альтернативный вариант (закомментирован)
		s.Q[i] = 0.0; // Квадратурная компонента отсутствует
	}
}

// Метод фазовой модуляции (PM)
void modulation::modPM2()
{
	for (int i = 0; i < sample; i++) { // Цикл по всем отсчетам
		s.I[i] = (bits[i] == 1) ? 1.0 : -1.0; // Фазовая модуляция (0 или 180 градусов)
		s.Q[i] = 0.0; // Квадратурная компонента отсутствует
	}
}

// Метод частотной модуляции (FM)
void modulation::modFM2()
{
	double accumulated_phase = 0.; // Накопленная фаза
	double df = bitrate / 4; // Девиация частоты
	for (int i = 0; i < sample; i++) { // Цикл по всем отсчетам
		accumulated_phase += (bits[i] == 1 ? df : -df) / (fd * 1000); // Изменение фазы
		s.I[i] = cos(accumulated_phase); // Синфазная компонента
		s.Q[i] = sin(accumulated_phase); // Квадратурная компонента
	}
}

// Метод получения сигнала
signal modulation::getS()
{
	return s; // Возврат сигнала
}

// Метод получения временных меток
std::vector<double> modulation::getT()
{
	return t; // Возврат временных меток
}

// Метод получения длительности сигнала
double modulation::getDuration()
{
	return duration; // Возврат длительности
}

// Метод получения битовой последовательности в виде вектора double
std::vector<double> modulation::getBits()
{
	std::vector<double> double_vec(bits.size()); // Вектор для результата

	for (size_t i = 0; i < bits.size(); ++i) { // Цикл по всем битам
		double_vec[i] = bits[i] ? 1.0 : 0.0; // Преобразование bool в double
	}
	return double_vec; // Возврат результата
}

// Основной метод модуляции
void modulation::manipulation()
{
	bits = generate_bits(nbits, sample); // Генерация битовой последовательности
	switch (type) // Выбор типа модуляции
	{
	case AM:
		modAM(); // Амплитудная модуляция
		break;
	case PM:
		modPM2(); // Фазовая модуляция
		break;
	case FM:
		modFM2(); // Частотная модуляция
		break;
	default:
		break;
	}
	for (int i = 0; i < sample; i++) { // Цикл по всем отсчетам
		s.s[i] = base(s.I[i], s.Q[i]); // Заполнение комплексного сигнала
	}
}

// Метод установки типа модуляции
void modulation::setType(type_modulation _type)
{
	type = _type; // Установка типа
}

// Метод создания базового сигнала (с вырезанием фрагмента)
signal modulation::createBaseSignal()
{
	signal result = s; // Копирование исходного сигнала
	result.erase(delay, sample_base); // Вырезание фрагмента сигнала
	s.N = sample; // Установка размера исходного сигнала
	result.N = sample_base; // Установка размера базового сигнала

	double phase = 0; // Фаза
	base temp; // Временная комплексная переменная
	for (int i = 0; i < s.N; i++) { // Цикл по всем отсчетам исходного сигнала
		phase += 2 * M_PI * deltaF / (fd * 1000); // Изменение фазы
		temp = base(cos(phase), sin(phase)); // Комплексная экспонента
		s.s[i] *= temp; // Умножение сигнала на комплексную экспоненту (перенос на несущую частоту)
	}
	normalizeSize(result, s); // Нормализация размеров сигналов
	return result; // Возврат базового сигнала
}