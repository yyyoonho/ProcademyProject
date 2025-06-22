#pragma once

struct stCharacter;
struct stSession;

// 섹터 하나당 크기 200 pixel * 200 pixel 32
#define dfSECTOR_MAX_Y 32
#define dfSECTOR_MAX_X 32

#define dfSECTOR_SIZE (6400/dfSECTOR_MAX_Y)

struct stSECTOR_POS
{
	int iY;
	int iX;
};

struct stSECTOR_AROUND
{
	int iCount;
	stSECTOR_POS around[9];
};

// 디버깅
void GetSectorAroundForAttack(BYTE direction, BYTE attackType, stCharacter* pCharacter, OUT stSECTOR_AROUND* pSectorAround);

void GetSectorAround(int iSectorY, int iSectorX, OUT stSECTOR_AROUND* pSectorAround);
void GetSessionsFromSector(int sectorY, int sectorX, OUT stSession* v[], OUT int* count);
void GetCharactersFromSector(int sectorY, int sectorX, OUT stCharacter* v[], OUT int* count);

void SetSector(stCharacter* pCharacter);

bool DeleteInSector(int sectorY, int sectorX, stCharacter* destroyCharacter);
bool UpdateSector(stCharacter* pCharacter);

void CharacterSectorUpdatePacket(stCharacter* pCharacter);
void GetUpdateSectorAround(stCharacter* pCharacter, OUT stSECTOR_AROUND* pRemoveSector, OUT stSECTOR_AROUND* pAddSector);
