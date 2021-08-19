#pragma once

// число цыферок у дисплейчика, в моём 4, бывают на 6 - 8, но у мня нет,я не тестил
//
const uint8_t NUM_DIGITS = 4;     


enum class enTM1637Type : bool { Number = false, Time = true };	// перечисление: тип дисплея числа/время
enum class enTM1637Align: bool {Left = false, Right = true};	// перечисление: выравнивание влево/вправа


class TM1637
{
private:
	static const uint8_t CMD_SET_DATA	= 0x40;  // следом папруть данные
	static const uint8_t CMD_SET_ADDR	= 0xC0;	 // следом пойдёт адрес
	static const uint8_t MAGIC_NUM		= 0x88;	 // особо не разбиралса, для чего это

protected:
	uint8_t FClockPin;  // пин для клока
	uint8_t FDataPin;	// пин для данных


	enTM1637Type FDisplayType : 1;		// тип дисплея, с точками или с двоеточием (для чисел/для времени)
	bool		FPointVisible : 1;		// видна ли точка/двоеточие при выводе
	uint8_t		FPointIndex	  : 3;		// позиция точки на экранчике 0..7
	uint8_t		FBrightness   : 3;		// яркость свечения дисплейчика 0..7

	uint8_t		FOutData[NUM_DIGITS];	// буфер выводимых данных (уже перекодированный под сегменты)
	uint8_t		*FSavedData;			// здесь будут сохраняться данные перед уходом в Sleep	


	void Start(void) const;   // выдает старт условие на шину

	void Stop(void) const;    // выдает стоп условие на шину

// выводит в шину 1 байт данных млатшым битом впердё
//
	void WriteByte(int8_t wr_data) const;

// вывод последовательно всего буфера + установка яркости
//
	void Update(void) const;


// вывод строки с выравниванием влево/вправо
// по умолчанию - выравнивание влево
// обычно строки выравниваются влево, цифры - вправо
// выводит не более NUM_DIGITS символов из строки, остальные игнорит
//
	void  OutString(const char *AString, const enTM1637Align AAlign = enTM1637Align::Left);

// На входе принимает букву/цифру, и ищет в PROGMEM сегментную маску для него
// возвращает маску символа, если такой есть, или 0х00, если символа в таблице нетю
//
	uint8_t GetSegments(const uint8_t ASymbol) const;

	TM1637() = delete;
//	TM1637(TM1637&) = delete;
//	TM1637(TM1637&&) = delete;  // ненужные конструкторы явно удалены

public:
// канструктор. Принимает 2 пина (обязательно) и тип дисплея (необязательно)
// AClockPin - номер пина для тактирования (вывод CLK на дисплейчике)
// ADataPin  - номер пина для данных (вывод DAT на дисплейчике)
// ADisplayType - тип дисплея, для чисел или для времени (с точками/двоеточием)
// по умолчанию - выбран дисплей с двоеточием (для времени)
//
	TM1637(uint8_t AClockPin, uint8_t ADataPin, enTM1637Type ADisplayType = enTM1637Type::Time);

// по многочисленным просьбам трудящихся добавил, хоть он не особо и нужен
// Если всё работает, специально вызывать этот метод НЕ нужно
//
	void Init(void);

// Перед отправкой контроллера в глубокий сон вызвать: 
//
	void Sleep(void);

// После пробуждения от сна вызвать
//
	void Wakeup(void);


// печатает первые NUM_DIGITS символов переданной строки
// с выравниванием. По умолчанию, выравнивание - влево
//
	void Print(const char *AString, const enTM1637Align AAlign = enTM1637Align::Left);

// печатает двухбайтовое целое число со знаком
// по основанию ARadix (по умолчанию 10)
// число выравнивается вправо
// печатается не более NUM_DIGITS начальных цыфр числа, включая знак
//
	void Print(const int ANumber, const uint8_t ARadix = 10);
	void Print(const unsigned ANumber, const uint8_t ARadix = 10);

	void Print(const long ANumber) { Print(int(ANumber), 10); };


// печать дробных чисел со знаком.
// c точностью APrecision знаков после запятой, по умолчанию - 1 
// Если число длинное - печатаются первые NUM_DIGITS символов
// Если индекс точки находится в пределах 0 - NUM_DIGITS то она тоже печатается
//
	void Print(const double AValue, const uint8_t APrecision = 1);

// для удобства печати времени, передаёшь часы/минуты, печатает c ведущими нулями 
// и на своих местах, слева и справа от двоеточия
//
	void PrintTime(const uint8_t AHours, const uint8_t AMinutes, const bool AShowPoint = true);

// для удобства. Передаешь число (со знаком) - печатает его и символ градуса за ним
//
	void PrintDeg(const int8_t ADegrees);

// очистка дисплея
//
	void Clear(void);

// устанавливает яркость дисплея от 0 до 7. 7 - самый яркий.
//
	void SetBrightness(const uint8_t AValue);

// включает/выключает показ десятичной точки
//
	void ShowPoint(const bool APointVisible);

	void ShowPointPos(const uint8_t APointPos, const bool AVisible = true);

// переключает точку. если была включена - выключает и наоборот
// удобно мигать двоеточием в часах, не надо самому запоминать состояние
//
	void ToggleColon(void);

// печать символа в определенном месте экрана
// APosition - в какой позиции печатать 0..NUM_DIGITS-1
// ASymbol - символ, который надо напечатать
// символ сразу же перекодируется и выводится в нужном месте
// ничего больше вызывать не надо
// 
	void PrintAt(const uint8_t APosition, const char ASymbol);

// аналогично Print(const char *AString)
// только оператором, чтоб можно было писать 
// tm1637<<"Err2";
//
	const TM1637 &operator <<(const char *AString) {
		OutString(AString);
		return *this;
}

};
