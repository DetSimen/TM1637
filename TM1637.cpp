#include "TM1637.h"
#include <Arduino.h>
#include <alloca.h>
#include <avr\pgmspace.h>

#pragma pack(push,1)

struct tagSegmentData {

public:
	char	Symbol;
	uint8_t Mask;
};

#pragma pack(pop)


static const tagSegmentData SegmentsData[]  PROGMEM {
	{ '0',0x3F},
	{ '1',0x06},
	{ '2',0x5B},
	{ '3',0x4F },
	{ '4',0x66 },
	{ '5',0x6D },
	{ '6',0x7D },
	{ '7',0x07 },
	{ '8',0x7F },
	{ '9',0x6F },
	{ 'A',0x77 },
	{ 'b',0x7c },
	{ 'C',0x39 },
	{ 'd',0x5E },
	{ 'E',0x79 },
	{ 'F',0x71 },
	{ ' ',0x00 },
	{ '-',0x40 }, 
	{ '*',0x63 },  // значок градуса, т.е пишем -25* выведетс€ -25 и значок градуса
	{ '_',0x08 },
	{ 'r',0x50 },
	{ 'H',0x76 },
	{ 'h',0x74 },
	{ 'L',0x38 },
	{ 'n',0x54 },
	{ 'o',0x5C },
	{ 'P',0x73 },
	{ 'S',0x6D },
	{ 'U',0x3E },
	{ 'u',0x1C },
	{ 'Y',0x6E }

};

static const uint8_t SEG_DATA_LENGHT = sizeof(SegmentsData) / sizeof(tagSegmentData);

/*
--0x01--
|        |
0x20     0x02
|        |
--0x40- -
|        |
0x10     0x04
|        |
--0x08--
*/


void TM1637::Start(void)    // выдает старт условие на шину
{
	digitalWrite(FClockPin, HIGH);
	digitalWrite(FDataPin, HIGH);
	digitalWrite(FDataPin, LOW);
	digitalWrite(FClockPin, LOW);
}


void TM1637::Stop(void) // выдает стоп условие на шину
{
	digitalWrite(FClockPin, LOW);
	digitalWrite(FDataPin, LOW);
	digitalWrite(FClockPin, HIGH);
	digitalWrite(FDataPin, HIGH);
}


void TM1637::WriteByte(int8_t wr_data) {


	for (uint8_t i = 0; i < 8; i++, wr_data >>= 1)
	{
		digitalWrite(FClockPin, LOW);
		digitalWrite(FDataPin, wr_data & 0x01);// млатшым битом вперЄд
		delayMicroseconds(3);

		digitalWrite(FClockPin, HIGH);
		delayMicroseconds(3);

	}
	digitalWrite(FClockPin, LOW);
	digitalWrite(FDataPin, HIGH);
	digitalWrite(FClockPin, HIGH);
}


void TM1637::Update(void) {
	Start();
	WriteByte(CMD_SET_DATA); // будем передавать данны€, NUM_DIGITS байт + управл€ющий байт, там где €ркость
	Stop();

	Start();
	WriteByte(CMD_SET_ADDR);
	for (uint8_t i = 0; i < NUM_DIGITS; i++) {
		uint8_t bytesend = FOutData[i];
		if (FPointVisible && (i == FPointIndex)) bytesend |= 0x80; // вывод точки в нужном месте
		WriteByte(bytesend);		// пишем байты из буфера
	}
	Stop();

	Start();
	WriteByte(MAGIC_NUM + FBrightness);  // пишем €ркость
	Stop();
}


void TM1637::OutString(const char * AString, const enTM1637Align AAlign) {

	Clear();
	char ch;
	if (AAlign == enTM1637Align::Left) {  // если выравнивание влево, печатаем с 0 индекса
		uint8_t idx = 0;
		while ((ch = *AString++) && (idx < NUM_DIGITS)) {
			FOutData[idx++] = GetSegments(ch);
		}
	}
	else {  // а если вправо - с конца. 
		uint8_t pos = NUM_DIGITS;
		uint8_t len = strlen(AString);
		if (len > NUM_DIGITS) len = NUM_DIGITS;

		while (pos > 0) {
			FOutData[--pos] = (len > 0) ? GetSegments(AString[--len]) : 0x00;
		}
	}

	Update();
}


uint8_t TM1637::GetSegments(const uint8_t ASymbol) {

	const tagSegmentData *src = &SegmentsData[0];

	for (uint8_t i = 0; i < SEG_DATA_LENGHT; src++, i++) {

       if (pgm_read_byte(&src->Symbol) == ASymbol) return pgm_read_byte(&src->Mask);

	}

	return 0x00;
}


TM1637::TM1637(uint8_t AClockPin, uint8_t ADataPin, enTM1637Type ADisplayType) {
	FClockPin = AClockPin;
	FDataPin = ADataPin;
	FDisplayType = ADisplayType;

	FPointIndex = (ADisplayType == enTM1637Type::Time) ? 1 : 7;
	FPointVisible = true;

	memset(FOutData, 0, NUM_DIGITS);

	pinMode(FClockPin, OUTPUT);
	pinMode(FDataPin, OUTPUT);

	Clear();
	SetBrightness(0x07);
}


void TM1637::Print(const char * AString) {

	OutString(AString, enTM1637Align::Left);

}


void TM1637::Print(const int ANumber, const uint8_t ARadix) {
	char *buf = alloca(16);

	itoa(ANumber, buf, ARadix);

	OutString(buf, enTM1637Align::Right);

	Update();
}


void TM1637::Print(const double AValue) {
	if (FDisplayType != enTM1637Type::Number) return;
	char *buf = alloca(12);

	dtostrf(AValue, NUM_DIGITS, 2, buf);

	int8_t point_idx = -1; 

	for (uint8_t i = 0; i < NUM_DIGITS; i++) {  // поиск позиции точки в строке 
		char ch = buf[i];
		if (ch == 0x00) break;
		if (ch != '.') continue;
		point_idx = i;
		break;
	}

	FPointIndex = (point_idx > 0) ? point_idx - 1 : 7;
	FPointVisible = (FPointIndex < NUM_DIGITS);

	OutString(buf, enTM1637Align::Right);

	Update();
}


void TM1637::PrintTime(const uint8_t AHours, const uint8_t AMinutes) {
	if (AHours > 23 || AMinutes > 59) return;

	Clear();

	FOutData[0] = GetSegments((AHours / 10) + '0');
	FOutData[1] = GetSegments((AHours % 10) + '0');
	FOutData[2] = GetSegments((AMinutes / 10) + '0');
	FOutData[3] = GetSegments((AMinutes % 10) + '0');
	FPointVisible = true;
	FPointIndex = 1;
	Update();
}


void TM1637::PrintDeg(const int8_t ADegrees) { // от -128 до +127

		char *buf = alloca(10);

		itoa(ADegrees, buf, 10);
		uint8_t len = strlen(buf);
		buf[len] = '*';			// безопасно. даже если передать отрицательную температуру
		buf[len + 1] = 0x00;	// меньше 100 гра, знак просто не выведеца и сЄ. 

		OutString(buf, enTM1637Align::Right);
}


void TM1637::Clear(void) {
	memset(FOutData, 0, NUM_DIGITS);
	FPointVisible = false;
	Update();
}


void TM1637::SetBrightness(const uint8_t AValue) {
	FBrightness = AValue & 0x07;
	Update();
}


void TM1637::ShowPoint(const bool APointVisible) {
	FPointVisible = APointVisible;
	Update();
}


void TM1637::ToggleColon(void) {
	if (FDisplayType == enTM1637Type::Time) {
		FPointIndex = 1;
		ShowPoint(not FPointVisible);

	}
}


void TM1637::PrintAt(const uint8_t APosition, const char ASymbol) {
	if (APosition < NUM_DIGITS) {
		FOutData[APosition] = GetSegments(ASymbol);
		Update();
	}
}
