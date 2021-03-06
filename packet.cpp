#include <iostream>
#include <iomanip>
#include <vector>
#include <sstream>
#include <cstring>
#include <math.h>

#include "packet.h"

using namespace std;


//====== Class PACKET ======

//static methods
bool Packet::isDoubleGpsLike(const double& iDouble)
{
    // check square where gps coordinates should point from our DAT files.
//    return (iDouble > 13.0 && iDouble < 102.0 && fabs(iDouble)>0.01);
return (iDouble > -1000 && iDouble < 3000 && fabs(iDouble)>0.01);
}

double Packet::convertDoubleToGps(const double& iDouble)
{
    return iDouble * 180.0 / 3.141592653589793; // 0x32 at github.com/noahwilliamsson/dji-phantom-vision/blob/master/dji-phantom.c
}

double Packet::doubleFromBytes(const uint8_t* iBytes)
{
    double res;
    memcpy(&res, iBytes, sizeof(double));
    return res;
}

float Packet::floatFromBytes(const uint8_t* iBytes)
{
    float res;
    memcpy(&res, iBytes, sizeof(float));
    return res;
}

const double Packet::doubleFromBytePos(int iPosition) const
{
    uint8_t aBytes[8]; // 8 - sizeof(double)

    // copy 8 bytes to aBytes
    for (int j = 0; j<sizeof(double); ++j)
    {
        aBytes[j] = _packv[j+iPosition];
    }
    // convert aBytes to double
   return  Packet::doubleFromBytes(aBytes);
}

const float Packet::floatFromBytePos(int iPosition) const
{
    uint8_t aBytes[4]; // 4 - sizeof(float)

    // copy 4 bytes to aBytes
    for (int j = 0; j<sizeof(float); ++j)
    {
        aBytes[j] = _packv[iPosition+j];
    }
    // convert aBytes to double
   return  Packet::floatFromBytes(aBytes);
}

string Packet::dateFromFourBytes(const uint8_t* iBytes)
{
    // NOTE: it's only for test purpose!
    // We are not sure that date and time envoded in this particular format!!!

    //Convert 4 bytes to datetime according to this rule
    // YYYYYYYMMMMDDDDDHHHHMMMMMMSSSSSS
    //    7     4   5    4    6     6  
    // stored as little endian 32bit unsigned integer
    
    unsigned int aValue;
    memcpy(&aValue, iBytes, sizeof(unsigned int)); 
    
    int sec = aValue &  0x3f; //0x3F(16) = 0011 1111 (2)
    aValue = aValue >> 6; // move to minutes
    int min = aValue & 0x3f;
    aValue = aValue >> 6; // move to hours
    int hour = aValue & 0xf; // 0xf = 1111
    aValue = aValue >> 4; // move to day
    int day = aValue & 0x1f; // 0x1f = 0001 1111
    aValue = aValue >> 5; // move to month
    int month = aValue & 0xf; // 0xf = 1111
    aValue = aValue >> 4; // ove to year
    int year = aValue & 0x7F; // 0x7F = 0011 1111
    stringstream ss;
    ss << day<<"/"<<month<<"/"<<year<<" "
       << hour<<":"<<min<<":"<<sec;
    return ss.str();
}

const string Packet::datetimeFromBytePos(int iPos) const
{
    uint8_t aBytes[4]; // 4 - sizeof(unsigned int)
    // copy 4 bytes to aBytes
    for (int j = 0; j<sizeof(float); ++j)
    {
        aBytes[j] = _packv[iPos+j];
    }
    // convert aBytes to double
   return  Packet::dateFromFourBytes(aBytes);

}

//member methods
void Packet::dataToDoubles()
{
    uint8_t aBytes[8]; // 8 - sizeof(double)

    for (int i = PACKET_HEADER_LEN; i<_packv.size()-7; ++i)
    {
        // copy 8 bytes to aBytes
        for (int j = 0; j<8; ++j)
        {
            aBytes[j] = _packv[i+j];
        }
        // convert aBytes to double
        _dataDoubles.push_back(doubleFromBytes(aBytes));
    }
}


string Packet::toString()
{
    stringstream ss;
    ss << "MsgID: " << dec << _msgId;
    ss << ", len: " << (int)_packetLen;
    ss << ", type: " << noshowbase << hex << setw(4) << setfill ('0') << _packetType << endl;
    /*
    for ( packetv_t::const_iterator it = _packv.begin(); it != _packv.end(); ++it)
    {
        ss << noshowbase << hex << setw(2) << setfill('0') << (int)(unsigned char)(*it) << " ";
    }
    */
    for (int i = 0; i< _packv.size(); ++i)
    {
        if (i==10 || i==18 || i==26) ss << endl;
        if (i==30 || i==62) ss <<endl << endl; // unknown part of the GPS message begins
        ss << noshowbase << hex << setw(2) << setfill('0') << (int)(unsigned char)(_packv[i]) << " ";
    }
    ss << dec << endl;
    return ss.str();
}


void Packet::parseHex()
{
    if (_packv.size() < PACKET_HEADER_LEN)
    {
        cerr << "Error while parsing message: message to short, size: " << dec <<_packv.size() << endl;
        _valid = false;
        return;
    }

    // byte 1 - packet len
    _packetLen = _packv[1];
    // packet len in byte1 must be the same as _packv vector length
    // otherwise consider that message is corrupted
    if (_packetLen != _packv.size() )
    {
        cerr << "Error while parsing message: message length in packet ("<< dec << (int)_packetLen
             << ") is not equal to number of bytes read from file("<< dec << _packv.size() << ")" << endl;
        _valid = false;
        return;
    }

    // bytes 3 and 4 - packet type (2 bytes)
    char aType[2];
    for (int i = 0; i<2; ++i) aType[i] = _packv[i+3];
    memcpy(&_packetType, aType, 2);

    // bytes 6-9 - message ID (4 bytes)
    char aMsgId[4];
    for (int i = 0; i<4; ++i)
    {
        aMsgId[i] = _packv[i+6];
    }
    memcpy(&_msgId, aMsgId, 4);

    _valid = true;

    // try to convert all data bytes into doubles
    dataToDoubles();
}



//====== class GPSPacket ======
void GPSPacket::parseHex()
{
    Packet::parseHex();
    
    // first we should check packet type (must be 0x01CF)
    if (this->getPacketType() != 0x01CF)
    {
     //   cout << "GPSPacket: Wrong packet type: " << hex << this->getPacketType();
        _valid = false;
        return;
    }

    // then check byte#6 (must be 0x00 for pgs packet)
    if (this->getPacketRaw()[6] != 0x00)
    {
     //   cout << "GPSPacket: Wrong byte#6 (muxt be 0x00) " << hex << this->getPacketRaw()[6] << endl;
        _valid = false;
        return;
    }

    // now we can get lat/long/alt values
    // TODO
    _lon=convertDoubleToGps(doubleFromBytePos(10)); // byte#10 - beginning of longitude
    _lat=convertDoubleToGps(doubleFromBytePos(18)); // byte#18 - beginning of latitude
    _alt=floatFromBytePos(26);

    _bytes30_33_f=floatFromBytePos(30);
    _bytes34_37_f=floatFromBytePos(34);
    _bytes38_41_f=floatFromBytePos(38);
    _bytes42_45_f=floatFromBytePos(42);
    
    _bytes46_49_f=floatFromBytePos(46);
    _bytes50_53_f=floatFromBytePos(50);
    _bytes54_57_f=floatFromBytePos(54);
    _bytes58_61_f=floatFromBytePos(58);
    
    
    _valid = true;

}

string GPSPacket::toString()
{
    stringstream ss;
    ss << "GPS Packet details: " << endl;
    ss << "Longitude: " << dec << fixed << _lon << endl;
    ss << "Latitude : " << dec << fixed << _lat << endl;
    ss << "Altitude : " << dec << fixed << _alt << " meters" << endl;
    ss << endl;
    ss << "_bytes30_33_f: " << dec<<fixed<<_bytes30_33_f
       << " time? - " << datetimeFromBytePos(30) <<endl; 
    ss << "_bytes34_37_f: " << dec<<fixed<<_bytes34_37_f 
       << " time? - " << datetimeFromBytePos(34) <<endl; 
    ss << "_bytes38_41_f: " << dec<<fixed<<_bytes38_41_f
       << " time? - " << datetimeFromBytePos(38) <<endl; 
    ss << "_bytes42_45_f: " << dec<<fixed<<_bytes42_45_f
       << " time? - " << datetimeFromBytePos(42) <<endl; 
    ss << endl;
    ss << "_bytes46_49_f: " << dec<<fixed<<_bytes46_49_f
        << " time? - " << datetimeFromBytePos(46) <<endl; 
    ss << "_bytes50_53_f: " << dec<<fixed<<_bytes50_53_f
        << " time? - " << datetimeFromBytePos(50) <<endl; 
    ss << "_bytes54_57_f: " << dec<<fixed<<_bytes54_57_f
        << " time? - " << datetimeFromBytePos(54) <<endl; 
    ss << "_bytes58_61_f: " << dec<<fixed<<_bytes58_61_f
        << " time? - " << datetimeFromBytePos(58) <<endl; 

    // Debug purpose:
    ss << "Packet details:"<<  endl << Packet::toString();
    ss << endl;
    return ss.str();
}

const string GPSPacket::toCsvString() const
{
    stringstream ss;
    ss << dec << fixed << _lat;
    ss << ","<< dec << fixed << _lon;
    ss << ","<< dec << fixed << _alt;
    
    ss << ","<< dec<<fixed<<_bytes30_33_f; 
    ss << ","<< dec<<fixed<<_bytes34_37_f; 
    ss << ","<< dec<<fixed<<_bytes38_41_f; 
    ss << ","<< dec<<fixed<<_bytes42_45_f; 
    ss << ","<< dec<<fixed<<_bytes46_49_f; 
    ss << ","<< dec<<fixed<<_bytes50_53_f; 
    ss << ","<< dec<<fixed<<_bytes54_57_f; 
    ss << ","<< dec<<fixed<<_bytes58_61_f; 
    
    ss << endl;
    return ss.str();
}


const string GPSPacket::csvHeader()
{
    return "Latitude,Longitude,Altitude,_bytes30_33_f,_bytes34_37_f,_bytes38_41_f,_bytes42_45_f,_bytes46_49_f,_bytes50_53_f,_bytes54_57_f,_bytes58_61_f\n";
}


