#include "garageApi.hpp"
#include "dbCallbacks.hpp"

#include <iostream>


GarageApi::GarageApi(sqlite3 *db):
    _db(db)
{
    _createDbTables();
}

GarageRetCode GarageApi::CreateGarage(uint levels, uint rowsPerLevel, uint spotsPerRow, GarageInfo_t &garageInfo)
{
    // ASSUMPTION: Each row contains spots of all the same type.
    // ASSUMPTION: There is a "random", even distribution of spot type.

    if (levels == 0U || rowsPerLevel == 0U || spotsPerRow == 0U)
    {
        return GarageRetCode::ERR_INVALID_ARGUMENTS;
    }

    // Create new garage and grab id
    std::string sql_statement = ""
        "INSERT INTO garages("
        "levels, rows_per_level, spots_per_row"
        ") VALUES (" +
        std::to_string(levels) + "," +
        std::to_string(rowsPerLevel) + "," +
        std::to_string(spotsPerRow) +
        ")";
    int db_ret_code = _run_sql_command(sql_statement);
    if (db_ret_code != 0)
    {
        return GarageRetCode::ERR_DATABASE;
    }
    int garage_id = sqlite3_last_insert_rowid(_db);
    // Create spots for new garage
    for (int level = 0; level < levels; level++)
    {
        std::vector<SpotType> spot_types{};
        for (uint row = 0; row < rowsPerLevel; row++)
        {
            // Reset spot type vector when empty
            if (spot_types.empty())
            {
                spot_types.push_back(SpotType::SPOT_MOTORCYCLE);
                spot_types.push_back(SpotType::SPOT_COMPACT);
                spot_types.push_back(SpotType::SPOT_LARGE);
            }
            // Pull "random" spot type from vector
            uint index = spot_types.size() > 1 ? rand() % (spot_types.size() - 1) : 0;
            SpotType spot_type = spot_types.at(index);
            spot_types.erase(spot_types.begin() + index);
            // Create row of spots of the pulled type
            for (uint spot_num = 0; spot_num < spotsPerRow; spot_num++)
            {
                // Create spot @ level, row, spot_num of specified spot type in newly created garage
                // e.g. level 2, row 5, spot 1, type LARGE, garage 3
                GarageRetCode ret_code = _createSpot(garage_id, level, row, spot_num, spot_type);
                if (ret_code != GarageRetCode::OK)
                {
                    return ret_code;
                }
            }
        }
    }
    // Fill in return info
    GarageRetCode ret_code = GetGarageInfo(garage_id, garageInfo);
    return ret_code;
}

GarageRetCode GarageApi::GetGarageInfo(int garageId, GarageInfo_t &garageInfo)
{
    std::string sql_statement;
    int db_ret_code;
    // Get basic garage info
    sql_statement = ""
        "SELECT id, levels, rows_per_level, spots_per_row"
        " FROM garages"
        " WHERE"
        " id = " + std::to_string(garageId);
    db_ret_code = _run_sql_command(sql_statement, dbCallbackGetGarageInfo, &garageInfo);
    if (db_ret_code != 0)
    {
        return GarageRetCode::ERR_DATABASE;
    }
    // Get vacant spots
    sql_statement = ""
        "SELECT id"
        " FROM parking_spots"
        " WHERE"
        " garage_id = " + std::to_string(garageId) +
        " AND parked_vehicle IS NULL";
    std::vector<int> spots_vacant{};
    db_ret_code = _run_sql_command(sql_statement, dbCallbackGetGarageInfoVector, &spots_vacant);
    if (db_ret_code != 0)
    {
        return GarageRetCode::ERR_DATABASE;
    }
    // Get filled spots
    sql_statement = ""
        "SELECT id"
        " FROM parking_spots"
        " WHERE"
        " garage_id = " + std::to_string(garageId) +
        " AND parked_vehicle IS NOT NULL";
    std::vector<int> spots_filled{};
    db_ret_code = _run_sql_command(sql_statement, dbCallbackGetGarageInfoVector, &spots_filled);
    if (db_ret_code != 0)
    {
        return GarageRetCode::ERR_DATABASE;
    }

    // Basic garage info is filled in during dbCallbackGetGarageInfo    
    garageInfo.spotsVacant = spots_vacant;
    garageInfo.spotsFilled = spots_filled;
    return GarageRetCode::OK;
}

GarageRetCode GarageApi::GetParkingSpotInfo(int parkingSpotId, ParkingSpotInfo_t &parkingSpotInfo)
{
    if (parkingSpotId < 0)
    {
        return GarageRetCode::ERR_INVALID_ID;
    }
    std::string sql_statement = ""
        "SELECT *"
        " FROM parking_spots"
        " WHERE id=" + std::to_string(parkingSpotId);
    ParkingSpotInfo_t parking_spot;
    int db_ret_code = _run_sql_command(sql_statement, dbCallbackGetParkingSpotInfo, &parking_spot);
    if (db_ret_code != 0)
    {
        std::cout << "Failure running sqlite3 command: " << sql_statement << std::endl;
        return GarageRetCode::ERR_DATABASE;
    }

    parkingSpotInfo = parking_spot;
    return GarageRetCode::OK;
}

GarageRetCode GarageApi::ParkVehicleInGarage(VehicleInfo_t vehicle, int garageId, int &parkingSpotId)
{
    // Get open spot for type
    int spot_id = _getVacantSpotId(vehicle.vehicleType);
    if (spot_id < 0)
    {
        return GarageRetCode::ERR_NO_VACANT_SPOT;
    }

    // Park vehicle in spot
    GarageRetCode ret_code = ParkVehicleInSpot(vehicle, spot_id);
    if (ret_code == GarageRetCode::OK)
    {
        parkingSpotId = spot_id;
    }
    return ret_code;
}

GarageRetCode GarageApi::ParkVehicleInSpot(VehicleInfo_t vehicle, int parkingSpotId)
{
    if (parkingSpotId < 0)
    {
        return GarageRetCode::ERR_INVALID_ID;
    }
    ParkingSpotInfo_t parking_spot;
    GarageRetCode ret_code = GetParkingSpotInfo(parkingSpotId, parking_spot);
    if (ret_code != GarageRetCode::OK)
    {
        return ret_code;
    }
    else if (parking_spot.spotType == SpotType::SPOT_NONE)
    {
        std::cout << "Invalid SpotType: " << parking_spot.spotType << std::endl;
        return GarageRetCode::ERR_INVALID_SPOT_TYPE;
    }
    else if (!parking_spot.isVacant)
    {
        std::cout << "Cannot park in spot (" << std::to_string(parkingSpotId) << "): Spot full!" << std::endl;
        return GarageRetCode::ERR_SPOT_FULL;
    }

    // Check type compatibility
    switch (vehicle.vehicleType)
    {
        case VehicleType::VEHICLE_MOTORCYCLE:
            // Motorcycles can park anywhere
            break;
        case VehicleType::VEHICLE_CAR:
            // Cars cannot park in motorcycle spots
            if (parking_spot.spotType == SpotType::SPOT_MOTORCYCLE)
            {
                ret_code = GarageRetCode::ERR_INVALID_SPOT;
            }
            break;
        case VehicleType::VEHICLE_BUS:
            // Busses have multiple conditions, see function
            ret_code = _checkParkingSpotBus(parking_spot);
            break;
        default:
            std::cout << "Invalid VehicleType: " << vehicle.vehicleType << std::endl;
            ret_code = GarageRetCode::ERR_INVALID_VEHICLE_TYPE;
    }

    if (ret_code == GarageRetCode::OK)
    {
        ret_code = _dbUpdateParkingSpot(parkingSpotId, vehicle.vehicleType);
    }
    return ret_code;
}

GarageRetCode GarageApi::_createSpot(int garageId, uint level, uint row, uint spot, SpotType spotType)
{
    std::string sql_statement = ""
        "INSERT INTO parking_spots("
        "garage_id, level, row, spot_num, spot_type"
        ") VALUES (" +
        std::to_string(garageId) + ", " +
        std::to_string(level) + ", " +
        std::to_string(row) + ", " +
        std::to_string(spot) + ", " +
        std::to_string(spotType) +
        ")";
    int db_ret_code = _run_sql_command(sql_statement);
    if (db_ret_code != 0)
    {
        std::cout << "Error creating garage spot." << std::endl;
        return GarageRetCode::ERR_DATABASE;
    }
    return GarageRetCode::OK;
}

int GarageApi::_getVacantSpotId(VehicleType vehicleType)
{
    int spot_id = -1;
    std::string sql_statement = ""
        "SELECT id, level, row, spot_num"
        " FROM parking_spots"
        " WHERE parked_vehicle IS NULL";
    // Get first ID that each vechicle can fit in
    switch(vehicleType)
    {
        case VehicleType::VEHICLE_MOTORCYCLE:
            // Can park anywhere that's open, no need to filter
            break;
        case VehicleType::VEHICLE_CAR:
            // Only large or compact spots, no motorcycle
            sql_statement += ""
                " AND spot_type in (" +
                std::to_string(SpotType::SPOT_COMPACT) + "," +
                std::to_string(SpotType::SPOT_LARGE) +
                ")";
            break;
        case VehicleType::VEHICLE_BUS:
            // Only large spots, no motorcycle or compact
            sql_statement += ""
                " AND spot_type = " + std::to_string(SpotType::SPOT_LARGE);
            break;
        default:
            std::cout << "Invalid VehicleType: " << vehicleType << std::endl;
            break;
    }
    sql_statement += ""
        " ORDER BY level ASC, row ASC, spot_num ASC";
    std::vector<ParkingSpotInfo_t> vacant_spots{};
    int db_ret_code =_run_sql_command(sql_statement, dbCallbackGetVacantParkingSpot, &vacant_spots);
    if (db_ret_code != 0)
    {
        return GarageRetCode::ERR_DATABASE;
    }

    if (!vacant_spots.empty())
    {
        switch(vehicleType)
        {
            // No special handling, just grab the first spot
            case VehicleType::VEHICLE_MOTORCYCLE:
            case VehicleType::VEHICLE_CAR:
                spot_id = vacant_spots[0].id;
                break;
            // Need to find 5 consecutive spots (same level & row)
            case VehicleType::VEHICLE_BUS:
                for (auto it = vacant_spots.begin(); it < vacant_spots.end() - 3;)
                {
                    bool spot_is_valid = false;
                    uint cur_level = it->level;
                    uint cur_row = it->row;
                    uint cur_spot_num = it->spotNum;
                    for (uint i = 1; i < 5; i++)
                    {
                        // If the row or level changes over the next 4 spots,
                        //  move to the changed spot and start again.
                        auto check_it = it + i;
                        if ((check_it->level != cur_level)
                            || (check_it->row != cur_row)
                            || (check_it->spotNum != (cur_spot_num + i)))
                        {
                            it = check_it;
                            spot_is_valid = false;
                            break;
                        }
                        spot_is_valid = true;
                    }
                    // Should only be true if the previous loop doesn't break,
                    //  meaning we have a valid bus spot.
                    if (spot_is_valid)
                    {
                        spot_id = it->id;
                        break;
                    }
                }
                break;
            default:
                std::cout << "Invalid VehicleType: " << vehicleType << std::endl;
                break;
        }
    }

    return spot_id;
}

GarageRetCode GarageApi::_checkParkingSpotBus(ParkingSpotInfo_t parkingSpot)
{
    if (parkingSpot.spotType != SpotType::SPOT_LARGE)
    {
        return GarageRetCode::ERR_INVALID_SPOT;
    }

    // Get all spots in same garage, same level, same row, same size, 
    std::string sql_statement = ""
        "SELECT id"
        " FROM parking_spots"
        " WHERE"
        " level = " + std::to_string(parkingSpot.level) +
        " AND row = " + std::to_string(parkingSpot.row) +
        " AND spot_num BETWEEN " + std::to_string(parkingSpot.spotNum) + " AND " + std::to_string(parkingSpot.spotNum + 4);
    std::vector<int> spots{};
    int db_ret_code = _run_sql_command(sql_statement, dbCallbackCheckParkingSpotBus, &spots);
    if (db_ret_code != 0)
    {
        return GarageRetCode::ERR_DATABASE;
    }
    else if (spots.size() != 5)
    {
        return GarageRetCode::ERR_INVALID_SPOT;
    }
    return GarageRetCode::OK;
}

GarageRetCode GarageApi::_dbUpdateParkingSpot(int parkingSpotId, VehicleType vehicleType)
{
    // ASSUMPTION: This function assumes the spot if valid, and in the case of
    // buses, that the next 4 consecutive row_id spots are also the next 4
    // consecutive spot_nums. (Garage generation ensures this for now)
    std::string sql_statement = ""
        "UPDATE parking_spots"
        " SET parked_vehicle = " + std::to_string(vehicleType);
    switch(vehicleType)
    {
        case VehicleType::VEHICLE_MOTORCYCLE:
        case VehicleType::VEHICLE_CAR:
            sql_statement += ""
                " WHERE id = " + std::to_string(parkingSpotId);
            break;
        case VehicleType::VEHICLE_BUS:
            sql_statement += ""
                " WHERE id BETWEEN " + std::to_string(parkingSpotId) + " AND " + std::to_string(parkingSpotId + 4);
            break;
        default:
            break;
    }
    int db_ret_code = _run_sql_command(sql_statement);
    if (db_ret_code != 0)
    {
        return GarageRetCode::ERR_DATABASE;
    }
    return GarageRetCode::OK;
}

int GarageApi::_run_sql_command(std::string sql_statement)
{
    _start_transaction();
    int db_ret_code = sqlite3_exec(_db, sql_statement.c_str(), NULL, NULL, NULL);
    if (db_ret_code != 0)
    {
        std::cout << "Failure running sqlite3 command: " << sql_statement << std::endl;
        _rollback_transaction();
    }
    _end_transaction();
    return db_ret_code;
}

int GarageApi::_run_sql_command(std::string sql_statement, int (*callback)(void*, int, char**, char**), void *passed)
{
    _start_transaction();
    int db_ret_code = sqlite3_exec(_db, sql_statement.c_str(), callback, passed, NULL);
    if (db_ret_code != 0)
    {
        std::cout << "Failure running sqlite3 command: " << sql_statement << std::endl;
        _rollback_transaction();
    }
    _end_transaction();
    return db_ret_code;
}

int GarageApi::_start_transaction()
{
    return sqlite3_exec(_db, "BEGIN TRANSACTION;", NULL, NULL, NULL);
}

int GarageApi::_rollback_transaction()
{
    return sqlite3_exec(_db, "ROLLBACK;", NULL, NULL, NULL);
}

int GarageApi::_end_transaction()
{
    return sqlite3_exec(_db, "END TRANSACTION;", NULL, NULL, NULL);
}

void GarageApi::_createDbTables()
{
    std::string sql_statement;
    sql_statement = ""
        "PRAGMA foreign_keys=ON;";
    _run_sql_command(sql_statement);
    sql_statement = ""
        "CREATE TABLE IF NOT EXISTS garages("
        " id INTEGER NOT NULL PRIMARY KEY AUTOINCREMENT,"
        " levels INTEGER NOT NULL,"
        " rows_per_level INTEGER NOT NULL,"
        " spots_per_row INTEGER NOT NULL"
        ")";
    _run_sql_command(sql_statement);
    sql_statement = ""
        "CREATE TABLE IF NOT EXISTS parking_spots("
        " id INTEGER NOT NULL PRIMARY KEY AUTOINCREMENT,"
        " spot_type INTEGER NOT NULL,"
        " parked_vehicle INTEGER,"
        " garage_id INTEGER NOT NULL,"
        " level INTEGER NOT NULL,"
        " row INTEGER NOT NULL,"
        " spot_num INTEGER NOT NULL,"
        " FOREIGN KEY(garage_id) REFERENCES garages(id) ON DELETE CASCADE,"
        " CONSTRAINT unq UNIQUE (garage_id, level, row, spot_num)"
        ")";
    _run_sql_command(sql_statement);
}

void GarageApi::_dropDbTables()
{
    std::string sql_statement;
    sql_statement = "DROP TABLE IF EXISTS garages";
    _run_sql_command(sql_statement);
    sql_statement = "DROP TABLE IF EXISTS parking_spots";
    _run_sql_command(sql_statement);
}

void GarageApi::Reset()
{
    _dropDbTables();
    _createDbTables();
}