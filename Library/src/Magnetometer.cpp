//
//  Magnetometer.cpp
//  Stonefish
//
//  Created by Patryk Cieslak on 29/03/2014.
//  Copyright (c) 2014 Patryk Cieslak. All rights reserved.
//

#include "Magnetometer.h"

Magnetometer::Magnetometer(std::string uniqueName, SolidEntity* attachment, btTransform relFrame, AxisType senseAxis, unsigned short resolution, unsigned int historyLength) : Sensor(uniqueName, historyLength)
{
    solid = attachment;
    relToSolid = relFrame;
    axis = senseAxis;
    bits = resolution;
    
    Reset();
}

void Magnetometer::Reset()
{
}

void Magnetometer::Update(btScalar dt)
{
    //calculate transformation from global to acc frame
    btMatrix3x3 toMagFrame = relToSolid.getBasis().inverse() * solid->getRigidBody()->getCenterOfMassTransform().getBasis().inverse();
    
    //magnetic north
    btVector3 north = toMagFrame * btVector3(1, 0, 0);
    
    //select axis
    btScalar mag = north[axis];
    
    //quantization
    mag = btScalar(trunc(mag * btScalar((1 << bits) - 1))) / btScalar((1 << bits) - 1); //quantization
    
    //save sample
    Sample s(1, &mag);
    AddSampleToHistory(s);
}

unsigned short Magnetometer::getNumOfDimensions()
{
    return 1;
}