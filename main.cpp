#include "garageApi.hpp"

#include <sqlite3.h>
#include <iostream>
#include <string>


bool testCreateGarage(GarageApi *api)
{
    api->Reset();
    bool is_success = true;
    GarageInfo_t garage_info;
    // Valid arguments
    is_success = is_success && (GarageRetCode::OK == api->CreateGarage(1, 2, 3, garage_info));
    // Check garage info
    is_success = is_success && (garage_info.levels == 1);
    is_success = is_success && (garage_info.rowsPerLevel == 2);
    is_success = is_success && (garage_info.spotsPerRow == 3);
    // 0 length levels, rows, or spots
    is_success = is_success && (GarageRetCode::ERR_INVALID_ARGUMENTS == api->CreateGarage(0, 2, 3, garage_info));
    is_success = is_success && (GarageRetCode::ERR_INVALID_ARGUMENTS == api->CreateGarage(1, 0, 3, garage_info));
    is_success = is_success && (GarageRetCode::ERR_INVALID_ARGUMENTS == api->CreateGarage(1, 2, 0, garage_info));
    // Report results
    std::string result = is_success ? "PASSED" : "FAILED";
    std::cout << "testCreateGarage: " << result << std::endl;
    return is_success;
}

bool testParkMotorcycle(GarageApi *api)
{
    api->Reset();
    bool is_success = true;
    // Create garage
    // 1 level with 3 rows of 1 spot each will guarantee each spot is of a different type
    GarageInfo_t garage_info;
    is_success = is_success && (GarageRetCode::OK == api->CreateGarage(1, 3, 1, garage_info));
    // Park 3 motorcycles
    int parking_spot_id;
    VehicleInfo_t vehicle = {VehicleType::VEHICLE_MOTORCYCLE};
    is_success = is_success && (GarageRetCode::OK == api->ParkVehicleInGarage(vehicle, garage_info.id, parking_spot_id));
    is_success = is_success && (GarageRetCode::OK == api->ParkVehicleInGarage(vehicle, garage_info.id, parking_spot_id));
    is_success = is_success && (GarageRetCode::OK == api->ParkVehicleInGarage(vehicle, garage_info.id, parking_spot_id));
    // // The 4th should fail
    is_success = is_success && (GarageRetCode::ERR_NO_VACANT_SPOT == api->ParkVehicleInGarage(vehicle, garage_info.id, parking_spot_id));
    // // Check garage info
    is_success = is_success && (GarageRetCode::OK == api->GetGarageInfo(garage_info.id, garage_info));
    is_success = is_success && (garage_info.spotsFilled.size() == 3);
    // Report results
    std::string result = is_success ? "PASSED" : "FAILED";
    std::cout << "testParkMotorcycle: " << result << std::endl;
    return is_success;
}

bool testParkCar(GarageApi *api)
{
    api->Reset();
    bool is_success = true;
    // Create garage
    // 1 level with 3 rows of 1 spot each will guarantee each spot is of a different type
    GarageInfo_t garage_info;
    is_success = is_success && (GarageRetCode::OK == api->CreateGarage(1, 3, 1, garage_info));
    // Park 2 cars
    int parking_spot_id;
    VehicleInfo_t vehicle = {VehicleType::VEHICLE_CAR};
    is_success = is_success && (GarageRetCode::OK == api->ParkVehicleInGarage(vehicle, garage_info.id, parking_spot_id));
    is_success = is_success && (GarageRetCode::OK == api->ParkVehicleInGarage(vehicle, garage_info.id, parking_spot_id));
    // The 3rd should fail (motorcycle spot)
    is_success = is_success && (GarageRetCode::ERR_NO_VACANT_SPOT == api->ParkVehicleInGarage(vehicle, garage_info.id, parking_spot_id));
    // Check garage info
    is_success = is_success && (GarageRetCode::OK == api->GetGarageInfo(garage_info.id, garage_info));
    is_success = is_success && (garage_info.spotsFilled.size() == 2);
    // Report results
    std::string result = is_success ? "PASSED" : "FAILED";
    std::cout << "testParkCar: " << result << std::endl;
    return is_success;
}

bool testParkBus(GarageApi *api)
{
    api->Reset();
    bool is_success = true;
    // Create garage big enough for 1 bus but not quite 2
    GarageInfo_t garage_info;
    is_success = is_success && (GarageRetCode::OK == api->CreateGarage(1, 3, 9, garage_info));
    // Park bus
    int parking_spot_id;
    VehicleInfo_t vehicle = {VehicleType::VEHICLE_BUS};
    is_success = is_success && (GarageRetCode::OK == api->ParkVehicleInGarage(vehicle, garage_info.id, parking_spot_id));
    // 2nd should fail
    is_success = is_success && (GarageRetCode::ERR_NO_VACANT_SPOT == api->ParkVehicleInGarage(vehicle, garage_info.id, parking_spot_id));
    // Check garage info
    is_success = is_success && (GarageRetCode::OK == api->GetGarageInfo(garage_info.id, garage_info));
    is_success = is_success && (garage_info.spotsFilled.size() == 5); // 5 per bus
    // Report results
    std::string result = is_success ? "PASSED" : "FAILED";
    std::cout << "testParkBus: " << result << std::endl;
    return is_success;
}

bool testParkingSpotInfo(GarageApi *api)
{
    api->Reset();
    bool is_success = true;
    ParkingSpotInfo_t parking_spot_info;
    // Create 2 level garage with enough room for 1 bus on each level
    GarageInfo_t garage_info;
    is_success = is_success && (GarageRetCode::OK == api->CreateGarage(2, 3, 5, garage_info));
    // Fill first level with...
    int parking_spot_id;
    VehicleInfo_t bus = {VehicleType::VEHICLE_BUS};
    VehicleInfo_t car = {VehicleType::VEHICLE_CAR};
    VehicleInfo_t motorcycle = {VehicleType::VEHICLE_MOTORCYCLE};
    // ...1 bus...
    is_success = is_success && (GarageRetCode::OK == api->ParkVehicleInGarage(bus, garage_info.id, parking_spot_id));
    // Check that a 2nd bus will park on the 2nd level
    is_success = is_success && (GarageRetCode::OK == api->ParkVehicleInGarage(bus, garage_info.id, parking_spot_id));
    is_success = is_success && (GarageRetCode::OK == api->GetParkingSpotInfo(parking_spot_id, parking_spot_info));
    is_success = is_success && (parking_spot_info.level == 1); // Zero-based
    // ...and 5 cars...
    for (int i = 0; i < 5; i++)
    {
        is_success = is_success && (GarageRetCode::OK == api->ParkVehicleInGarage(car, garage_info.id, parking_spot_id));
    }
    // ...and 5 motorcycles
    for (int i = 0; i < 5; i++)
    {
        is_success = is_success && (GarageRetCode::OK == api->ParkVehicleInGarage(motorcycle, garage_info.id, parking_spot_id));
    }
    // Check that a 6th motorcycle will park on the 2nd level
    is_success = is_success && (GarageRetCode::OK == api->ParkVehicleInGarage(motorcycle, garage_info.id, parking_spot_id));
    is_success = is_success && (GarageRetCode::OK == api->GetParkingSpotInfo(parking_spot_id, parking_spot_info));
    is_success = is_success && (parking_spot_info.level == 1); // Zero-based
    // Check garage info
    is_success = is_success && (GarageRetCode::OK == api->GetGarageInfo(garage_info.id, garage_info));
    is_success = is_success && (garage_info.spotsFilled.size() == 21); // 2 buses (10) + 5 cars (5) + 6 motorcycles (6)
    // Report results
    std::string result = is_success ? "PASSED" : "FAILED";
    std::cout << "testParkingSpotInfo: " << result << std::endl;
    return is_success;
}


int main(int argc, char **argv)
{
    std::string db_path = "./garages.db3";
    if (argc == 2)
    {
        db_path = std::string(argv[1]);
    }

    sqlite3 *db;
    int dbRetCode = sqlite3_open(db_path.c_str(), &db);
    if (dbRetCode)
    {
        std::cout << "Can't open database file: " << db_path << std::endl;
        std::cout << "Error code: " << sqlite3_errmsg(db) << std::endl;
    }

    GarageApi *api = new GarageApi(db);

    testCreateGarage(api);
    testParkMotorcycle(api);
    testParkCar(api);
    testParkBus(api);
    testParkingSpotInfo(api);

    delete api;
    return 0;
}