#pragma once

bool MoveSector(INT64 accountNo, WORD newSectorY, WORD newSectorX, WORD oldSectorY, WORD oldSectorX);
void GetAdjacentSectorPlayers(WORD sectorY, WORD sectorX, OUT std::vector<INT64>& arr);

void CreatePlayerToSector(INT64 accountNo, WORD sectorY, WORD sectorX);
bool RemovePlayerFromSector(INT64 accountNo, WORD sectorY, WORD sectorX);