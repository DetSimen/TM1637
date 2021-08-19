#include <Arduino.h>
#include "TM1637.h"
#include <alloca.h>
#include <avr\pgmspace.h>


#pragma pack(push,1)

struct T1637SegmentData {

public:
	char	Symbol;
	uint8_t Mask;
};

#pragma pack(pop)

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



static const T1637SegmentData SegmentsData[]  PROGMEM {
	{ '0',0x3F },
	{ '1',0x06 },
	{ '2',0x5B },
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
	{ '*',0x63 },  // значок градуса, т.е пишем -25* выведется -25 и значок градуса
	{ '_',0x08 },
	{ 'r',0x50 },
	{ 'H',0x76 },
	{ 'I',0x06 },
	{ 'h',0x74 },
	{ 'L',0x38 },
	{ 'n',0x54 },
	{ 'o',0x5C },
	{ 't',0x78 },
	{ 'O',0x3F },
	{ 'P',0x73 },
	{ 'S',0x6D },
	{ 'U',0x3E },
	{ 'u',0x1C },
	{ 'Y',0x6E }

};

static const uint8_t SEG_DATA_SIZE = sizeof(SegmentsData) / sizeof(T1637SegmentData);


void TM1637::Start(void) const    // выдает старт условие на шину
{
	digitalWrite(FClockPin, HIGH);
	digitalWrite(FDataPin, HIGH);
	digitalWrite(FDataPin, LOW);
	digitalWrite(FClockPin, LOW);
}


void TM1637::Stop(void) const // выдает стоп условие на шину
{
	digitalWrite(FClockPin, LOW);
	digitalWrite(FDataPin, LOW);
	digitalWrite(FClockPin, HIGH);
	digitalWrite(FDataPin, HIGH);
}


void TM1637::WriteByte(int8_t wr_data) const {


	for (uint8_t i = 0; i < 8; i++, wr_data >>= 1)
	{
		digitalWrite(FClockPin, LOW);
		digitalWrite(FDataPin, wr_data & 0x01);// млатшым битом вперёд
		delayMicroseconds(8);

		digitalWrite(FClockPin, HIGH);
		delayMicroseconds(8);

	}

	digitalWrite(FClockPin, LOW);
	delayMicroseconds(8);
	digitalWrite(FDataPin, HIGH);
	delayMicroseconds(8);
	digitalWrite(FClockPin, HIGH);
	delayMicroseconds(8);
}


void TM1637::Update(void) const {
	Start();
	WriteByte(CMD_SET_DATA); // будем передавать данныя, NUM_DIGITS байт + управляющий байт, там где яркость
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
	WriteByte(MAGIC_NUM + FBrightness);  // пишем яркость
	Stop();
}


void TM1637::OutString(const char * AString, const enTM1637Align AAlign) {

	Clear();

	if (AAlign == enTM1637Align::Left) {

		for (uint8_t i = 0; i < NUM_DIGITS; i++) {
			char ch = AString[i];
			if (ch == 0x00) break;
			FOutData[i] = GetSegments(ch);
		}
	}
	else {
		int8_t len = strlen(AString);
		if (len > NUM_DIGITS) len = NUM_DIGITS;
		for (int8_t i = NUM_DIGITS; (i >= 0) && (len > 0); ) {
			FOutData[--i] = GetSegments(AString[--len]);
		}

	}


	Update();
}


uint8_t TM1637::GetSegments(const uint8_t ASymbol) const {

	const T1637SegmentData *src = &SegmentsData[0];

	for (uint8_t i = 0; i < SEG_DATA_SIZE; src++, i++) {

	   if (pgm_read_byte(&src->Symbol) == ASymbol) 
		   return pgm_read_byte(&src->Mask);
	}

	return 0; // не нашли такого символа, отдаем нулевую маску
}


TM1637::TM1637(uint8_t AClockPin, uint8_t ADataPin, enTM1637Type ADisplayType) {
	FClockPin = AClockPin;
	FDataPin = ADataPin;
	FDisplayType = ADisplayType;

	FPointIndex = (ADisplayType == enTM1637Type::Time) ? 1 : 7;
	FPointVisible = true;

	memset(FOutData, 0x00, NUM_DIGITS);

	Init();
	SetBrightness(0x03);
}

void TM1637::Init(void)
{
	pinMode(FClockPin, OUTPUT);
	pinMode(FDataPin, OUTPUT);
	Stop();

	Clear();
}

void TM1637::Sleep(void)
{
	if (FSavedData == NULL) FSavedData = new uint8_t[NUM_DIGITS];
	if (FSavedData != NULL) memcpy(FSavedData, FOutData, NUM_DIGITS);
	Clear();

	pinMode(FClockPin, INPUT);
	pinMode(FDataPin, INPUT);
}

void TM1637::Wakeup(void)
{
	Init();
	if (FSavedData != NULL) {
		memcpy(FOutData, FSavedData, NUM_DIGITS);
		delete[] FSavedData;
		FSavedData = NULL;
		Update();
	}
}


void TM1637::Print(const char *AString, const enTM1637Align AAlign) {

	OutString(AString, AAlign);

}


void TM1637::Print(const int ANumber, const uint8_t ARadix) {
	char *buf = (char *)(alloca(16));

	itoa(ANumber, buf, ARadix);
	
	OutString(buf, enTM1637Align::Right);

}

void TM1637::Print(const unsigned ANumber, const uint8_t ARadix)
{
	char* buf = (char*)(alloca(16));

	utoa(ANumber, buf, ARadix);

	OutString(buf, enTM1637Align::Right);
}


void TM1637::Print(const double AValue, const uint8_t APrecision) {
	if (FDisplayType != enTM1637Type::Number) return;
	char *buf = (char *)(alloca(12));

	dtostrf(AValue, NUM_DIGITS+1, APrecision, buf);

	uint8_t len = strlen(buf);
	char *out = (char *)(alloca(len + 1));

	uint8_t pt_idx = 255;
	
	uint8_t n = 0;
	for (uint8_t i = 0; i < len; i++) {
		char ch = buf[i];
		if (ch != '.')
			out[n++] = ch;
		else
			pt_idx = i - 1;
	}

	OutString(out, enTM1637Align::Right);

	FPointVisible = pt_idx < NUM_DIGITS;

	if (FPointVisible) {
		FPointIndex = pt_idx;
		Update();
	}
}


void TM1637::PrintTime(const uint8_t AHours, const uint8_t AMinutes, const bool AShowPoint) {

	Clear();

	FOutData[0] = GetSegments((AHours / 10) + '0');
	FOutData[1] = GetSegments((AHours % 10) + '0');
	FOutData[2] = GetSegments((AMinutes / 10) + '0');
	FOutData[3] = GetSegments((AMinutes % 10) + '0');
	FPointVisible = AShowPoint;
	FPointIndex = 1;
	Update();
}


void TM1637::PrintDeg(const int8_t ADegrees) { // от -128 до +127

		char *buf = (char *)(alloca(10));

		itoa(ADegrees, buf, 10);
		uint8_t len = strlen(buf);
		buf[len] = '*';			// безопасно. даже если передать отрицательную температуру
		buf[len + 1] = 0x00;	// меньше 100 гра, знак градуса просто не выведеца и сё. 

		FPointVisible = false;
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

void TM1637::ShowPointPos(const uint8_t APointPos, const bool AVisible)
{
	FPointIndex = APointPos;
	FPointVisible = AVisible;
	Update();
}


void TM1637::ToggleColon(void) {

	if (FDisplayType == enTM1637Type::Time) FPointIndex = 1;

	ShowPoint(not FPointVisible);
}


void TM1637::PrintAt(const uint8_t APosition, const char ASymbol) {
	if (APosition < NUM_DIGITS) {
		FOutData[APosition] = GetSegments(ASymbol);
		Update();
	}
}
