/*
 * Garage API definitions.
 * 
 * Functions that provide access to creating and populating parking garages.
 */
#pragma once

#include <sqlite3.h>
#include <unordered_map>
#include <vector>


enum GarageRetCode {
    OK = 0,
    ERR_DATABASE,
    ERR_INVALID_ARGUMENTS,
    ERR_INVALID_ID,
    ERR_INVALID_SPOT,
    ERR_INVALID_SPOT_TYPE,
    ERR_INVALID_VEHICLE_TYPE,
    ERR_NO_VACANT_SPOT,
    ERR_SPOT_FULL,
};

enum SpotType {
    SPOT_NONE = 100,
    SPOT_MOTORCYCLE,
    SPOT_COMPACT,
    SPOT_LARGE,
};

enum VehicleType {
    VEHICLE_NONE = 200,
    VEHICLE_MOTORCYCLE,
    VEHICLE_CAR,
    VEHICLE_BUS,
};

typedef struct GarageInfo_t {
    int id              = -1;
    int levels          = 0;
    uint rowsPerLevel   = 0;
    uint spotsPerRow    = 0;
    std::vector<int> spotsFilled{};
    std::vector<int> spotsVacant{};
} GarageInfo_t;

inline std::ostream &operator<<(std::ostream &os, const GarageInfo_t &value)
{
    printf("Garage Info:\n");
    printf("\tid: %d\n", value.id);
    printf("\tnum vacant: %lu\n", value.spotsVacant.size());
    printf("\tnum filled: %lu\n", value.spotsFilled.size());
    return os;
}

typedef struct ParkingSpotInfo_t {
    int  id         = -1;
    int  garageId   = 0;
    uint level      = 0;
    uint row        = 0;
    uint spotNum    = 0;
    bool isVacant   = false;
    SpotType spotType = SPOT_NONE;
    VehicleType parkedVehicle = VEHICLE_NONE;
} ParkingSpotInfo_t;

typedef struct VehicleInfo_t {
    VehicleType vehicleType = VEHICLE_NONE;
} VehicleInfo_t;

class GarageApi
{
public:
    /**
     * Create an API for parking garage manipulation.
     * 
     * @param db A reference to and already open sqlite3 database.
     * @return API object.
     */
    GarageApi(sqlite3 *db);
    ~GarageApi(){};

    /**
     * Create a new garage entry and accompanying parking spot entries
     *  determined by the provided dimensions.
     * 
     * @param levels Number of parking garage levels.
     * @param rowsPerLevel Number of rows in each parking garage level.
     * @param spotsPerRow Number of parking spots in each row.
     * @param garageInfo (OUT) Struct populated with info of the newly created parking garage.
     * @return relevant return code.
     */
    GarageRetCode CreateGarage(uint levels, uint rowsPerLevel, uint spotsPerRow, GarageInfo_t &garageInfo);
    /**
     * Populate a struct with the dimension and vacancy info of a requested
     *  parking garage.
     * 
     * @param garageId ID of the requested parking garage.
     * @param garageInfo (OUT) Struct populated with info of the requested parking garage.
     * @return relevant return code.
     */
    GarageRetCode GetGarageInfo(int garageId, GarageInfo_t &garageInfo);
    /**
     * Populate a struct with the location and vacancy info of a requested
     *  parking spot.
     * 
     * @param parkingSpotId ID of the requested parking spot.
     * @param parkingSpotInfo (OUT) Struct populated with info of the requested parking spot.
     * @return relevant return code.
     */
    GarageRetCode GetParkingSpotInfo(int parkingSpotId, ParkingSpotInfo_t &parkingSpotInfo);
    /**
     * Attempt to park a vehicle in the requested parking garage. The first compatible
     *  and empty spot will be chosen. Will return an error if no spots can be found.
     * 
     * @param parkingSpotId ID of the requested parking spot.
     * @param parkingSpotInfo (OUT) Struct populated with info of the requested parking spot.
     * @return relevant return code.
     */
    GarageRetCode ParkVehicleInGarage(VehicleInfo_t vehicle, int garageId, int &parkingSpotId);
    /**
     * Attempt to park a vehicle in the requested parking spot. Will return an
     *  error if spot is full or does not match the type of vehicle provided. 
     * 
     * @param parkingSpotId ID of the requested parking spot.
     * @param parkingSpotInfo (OUT) Struct populated with info of the requested parking spot.
     * @return relevant return code.
     */
    GarageRetCode ParkVehicleInSpot(VehicleInfo_t vehicle, int parkingSpotId);
    /**
     * Drops and re-creates the garages and parking_spots tables of the database.
     * 
     * This was for quick testing and SHOULD NEVER make it into a production release.
     */
    void Reset();

private:
    void    _createDbTables();
    void    _dropDbTables();
    int     _run_sql_command(std::string sql_statement);
    int     _run_sql_command(std::string sql_statement, int (*callback)(void*, int, char**, char**), void *passed);
    int     _start_transaction();
    int     _rollback_transaction();
    int     _end_transaction();
    int     _getVacantSpotId(VehicleType vehicleType);
    GarageRetCode _createSpot(int garageId, uint level, uint row, uint spotNum, SpotType spotType);
    GarageRetCode _checkParkingSpotBus(ParkingSpotInfo_t parkingSpot);
    GarageRetCode _dbUpdateParkingSpot(int parkingSpotId, VehicleType vehicleType);

    sqlite3 *_db;
};