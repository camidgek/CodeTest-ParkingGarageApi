/*
 * Database call back functions.
 * 
 * Each function is called once per row of the return contents of a given SQLITE command.
 */
#pragma once

#include <string>
#include <iostream>


static int dbCallbackGetGarageInfo(void *pGarageInfo, int count, char **data, char **columns)
{
    if (count != 4)
    {
        std::cout << "ERR: dbCallbackGetGarageInfo: Schema was updated and count is invalid." << std::endl;
        return -1;
    }
    GarageInfo_t *garage = static_cast<GarageInfo_t*>(pGarageInfo);
    garage->id = std::stoi(data[0]);
    garage->levels = std::stoi(data[1]);
    garage->rowsPerLevel = std::stoi(data[2]);
    garage->spotsPerRow = std::stoi(data[3]);
    return 0;
}

static int dbCallbackGetGarageInfoVector(void *pSpots, int count, char **data, char **columns)
{
    if (count != 1)
    {
        std::cout << "ERR: dbCallbackGetGarageInfoVector: Schema was updated and count is invalid." << std::endl;
        return -1;
    }
    std::vector<int> *spots = static_cast<std::vector<int>*>(pSpots);
    spots->push_back(std::stoi(data[0]));
    return 0;
}

static int dbCallbackGetParkingSpotInfo(void *pParkingSpotInfo, int count, char **data, char **columns)
{
    if (count != 7)
    {
        std::cout << "ERR: dbCallbackGetParkingSpotInfo: Schema was updated and count is invalid." << std::endl;
        return -1;
    }
    ParkingSpotInfo_t *parking_spot = static_cast<ParkingSpotInfo_t*>(pParkingSpotInfo);
    parking_spot->id = std::stoi(data[0]);
    parking_spot->spotType = static_cast<SpotType>(std::stoi(data[1]));
    parking_spot->parkedVehicle = (data[2] == nullptr) ? VEHICLE_NONE : static_cast<VehicleType>(std::stoi(data[2]));
    parking_spot->isVacant = (data[2] == nullptr) ? true : false;
    parking_spot->garageId = std::stoi(data[3]);
    parking_spot->level = std::stoi(data[4]);
    parking_spot->row = std::stoi(data[5]);
    parking_spot->spotNum = std::stoi(data[6]);
    return 0;
}

static int dbCallbackGetVacantParkingSpot(void *pVacantSpots, int count, char **data, char **columns)
{
    if (count != 4)
    {
        std::cout << "ERR: dbCallbackGetVacantParkingSpot: Schema was updated and count is invalid." << std::endl;
        return -1;
    }
    std::vector<ParkingSpotInfo_t> *vacant_spots = static_cast<std::vector<ParkingSpotInfo_t>*>(pVacantSpots);
    ParkingSpotInfo_t parking_spot = {};
    parking_spot.id = std::stoi(data[0]);
    parking_spot.level = std::stoi(data[1]);
    parking_spot.row = std::stoi(data[2]);
    parking_spot.spotNum = std::stoi(data[3]);
    vacant_spots->push_back(parking_spot);
    return 0;
}

static int dbCallbackCheckParkingSpotBus(void *pSpots, int count, char **data, char **columns)
{
    if (count != 1)
    {
        std::cout << "ERR: dbCallbackCheckParkingSpotBus: Schema was updated and count is invalid." << std::endl;
        return -1;
    }
    std::vector<int> *spots = static_cast<std::vector<int>*>(pSpots);
    spots->push_back(std::stoi(data[0]));
    return 0;
}
