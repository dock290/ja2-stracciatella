#include "StrategicMap_Secrets.h"
#include "Buffer.h"
#include "ContentManager.h"
#include "GameInstance.h"
#include "LoadSaveData.h"
#include "SamSiteModel.h"
#include "StrategicMapSecretModel.h"
#include "TownModel.h"

std::map<uint8_t, BOOLEAN> isSecretFound;
Observable<UINT8> OnMapSecretFound;

void InitMapSecrets()
{
	isSecretFound.clear();
	for (auto s : GCM->getMapSecrets())
	{
		isSecretFound[s->sectorID] = FALSE;
	}
}

// only considers the town's base sector
BOOLEAN IsTownFound(INT8 const bTownID)
{
	auto town = GCM->getTown(bTownID);
	if (!town)
	{
		STLOGW("Town #{} not found", bTownID);
		return FALSE;
	}

	if (isSecretFound.find(town->getBaseSector()) == isSecretFound.end())
	{
		// town is always known to the player
		return TRUE;
	}

	return IsSecretFoundAt(town->getBaseSector());
}

BOOLEAN IsSecretFoundAt(UINT8 const sectorID)
{
	if (isSecretFound.find(sectorID) == isSecretFound.end())
	{
		// The game always try to find secrets at J9 and K4, but they
		// may not be present in a modded set up. So we will just
		// return TRUE and continue.
		STLOGW("No secret defined at sector {}", sectorID);
		return TRUE;
	}
	return isSecretFound[sectorID];
}

void SetTownAsFound(INT8 const bTownID, BOOLEAN const fFound)
{
	auto town = GCM->getTown(bTownID);
	if (!town)
	{
		// The game has hard-coded references to TIXA and
		// ORTA, but they may not be present in a modded
		// set up.
		STLOGW("Town #{} is not defined", bTownID);
		return;
	}

	SetSectorSecretAsFound(town->getBaseSector(), fFound);
}

void SetSAMSiteAsFound( UINT8 uiSamIndex )
{
	// set this SAM site as being found by the player
	const SamSiteModel* samSite = GCM->getSamSites()[uiSamIndex];
	SetSectorSecretAsFound(samSite->sectorId);
}

void SetSectorSecretAsFound(UINT8 const ubSectorID, BOOLEAN const fFound)
{
	isSecretFound[ubSectorID] = fFound;
	if (fFound)
	{
		OnMapSecretFound(ubSectorID);
	}
}

const StrategicMapSecretModel* GetMapSecretBySectorID(UINT8 const ubSectorID)
{
	for (auto s : GCM->getMapSecrets())
	{
		if (s->sectorID == ubSectorID) return s;
	}
	return NULL;
}


void InjectSAMSitesFoundToSavedFile(DataWriter& d)
{
	std::vector<BOOLEAN> fSAMFound;
	for (auto s : GCM->getMapSecrets())
	{
		if (s->isSAMSite)
		{
			BOOLEAN fFound = IsSecretFoundAt(s->sectorID);
			fSAMFound.push_back(fFound);
		}
	}
	INJ_BOOLA(d, fSAMFound.data(), fSAMFound.size());
}

void ExtractSAMSitesFoundFromSavedFile(DataReader& d)
{
	std::vector<const StrategicMapSecretModel*> secretSAMSites;
	for (auto s : GCM->getMapSecrets())
	{
		if (s->isSAMSite) secretSAMSites.push_back(s);
	}

	auto len = secretSAMSites.size();
	SGP::Buffer<BOOLEAN> fSAMFound(len);
	EXTR_BOOLA(d, fSAMFound, len)

	int i = 0;
	for (auto s : secretSAMSites)
	{
		SetSectorSecretAsFound(s->sectorID, fSAMFound[i++]);
	}
}

void InjectMapSecretStateToSave(DataWriter& d, unsigned int const index)
{
	INJ_BOOL(d, GetMapSecretStateForSave(index))
}

BOOLEAN GetMapSecretStateForSave(unsigned int const index)
{
	unsigned int i = 0;
	for (auto s : GCM->getMapSecrets())
	{
		if (s->isSAMSite) continue;
		if (index == i++)
		{        // we have found the secret to save
			return IsSecretFoundAt(s->sectorID);
		}
	}
	STLOGW("There is no secret at slot #{}", index);
	return FALSE;  // write something to maintain save compatibility
}

void ExtractMapSecretStateFromSave(DataReader & d, unsigned int const index)
{
	BOOLEAN fFound;
	EXTR_BOOL(d, fFound)
	SetMapSecretStateFromSave(index, fFound);
}

void SetMapSecretStateFromSave(unsigned int const index, BOOLEAN const fFound)
{
	unsigned int i = 0;
	for (auto s : GCM->getMapSecrets())
	{
		if (s->isSAMSite) continue;
		if (index == i++)
		{	// we have found the secret to load
			SetSectorSecretAsFound(s->sectorID, fFound);
			return;
		}
	}
	STLOGW("There is no secret at slot #{}", index);
}
