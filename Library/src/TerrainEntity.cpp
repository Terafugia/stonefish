//
//  TerrainEntity.cpp
//  Stonefish
//
//  Created by Patryk Cieslak on 1/9/13.
//  Copyright (c) 2013 Patryk Cieslak. All rights reserved.
//

#include "TerrainEntity.h"

#include "OpenGLUtil.h"

TerrainEntity::TerrainEntity(std::string uniqueName, int width, int length, btScalar size, btScalar minHeight, btScalar maxHeight, btScalar roughness, Material* mat, Look l, const btTransform& worldTransform) : Entity(uniqueName)
{
    wireframe = false;
    material = mat;
    look = l;
    
    size = UnitSystem::SetLength(size);
    minHeight = UnitSystem::SetLength(minHeight);
    maxHeight = UnitSystem::SetLength(maxHeight);
    
    terrainHeight = new btScalar[width*length];
    btVector3* terrainNormals = new btVector3[width*length];
    btScalar correl = 1000;
    btScalar amplitude = maxHeight;
    
    //heightfield generation
    for(int i=0; i<length; i++)
        for(int h=0; h<width; h++)
        {
            btScalar distance = sqrt((i*size)*(i*size)+(h*size)*(h*size))*10;
            btScalar height = amplitude*sin(i*size*0.5)*cos(h*size*0.5)*exp(-(distance/correl)*(distance/correl));   //amplitude*amplitude*exp(-fabs(distance/correl)*fabs(distance/correl))+minHeight;
            terrainHeight[i*width+h] = height > maxHeight ? maxHeight : height;
        }
    
    //normals calculation
    for(int i=0; i<length; i++)
        for(int h=0; h<width; h++)
            terrainNormals[i*width+h] = btVector3(0,1,0);
    
    for(int i=1; i<length-1; i++)
        for(int h=1; h<width-1; h++)
        {
            btVector3 c = btVector3(h*size, terrainHeight[i*width+h], i*size);
            btVector3 a1 = btVector3((h-1)*size, terrainHeight[i*width+(h-1)], i*size) - c;
            btVector3 a2 = btVector3((h+1)*size, terrainHeight[i*width+(h+1)], i*size) - c;
            btVector3 a3 = btVector3(h*size, terrainHeight[(i-1)*width+h], (i-1)*size) - c;
            btVector3 a4 = btVector3(h*size, terrainHeight[(i+1)*width+h], (i+1)*size) - c;
            terrainNormals[i*width+h] = (a1.cross(a3).normalized()+a3.cross(a2).normalized()+a2.cross(a4).normalized()+a4.cross(a1).normalized()).normalized()*-1;
        }
    
    //rigid body creation
    btDefaultMotionState* motionState = new btDefaultMotionState(UnitSystem::SetTransform(worldTransform));
    int upAxis = 1;
    btHeightfieldTerrainShape* shape = new btHeightfieldTerrainShape(width, length, terrainHeight, 1.0, minHeight, maxHeight, upAxis, PHY_FLOAT, false);
    btVector3 localScaling = btVector3(size, 1.0, size);
	shape->setLocalScaling(localScaling);
    shape->setUseDiamondSubdivision(true);
    
    btRigidBody::btRigidBodyConstructionInfo rigidBodyCI(0, motionState, shape, btVector3(0,0,0));
    rigidBodyCI.m_friction = rigidBodyCI.m_rollingFriction = rigidBodyCI.m_restitution = btScalar(1.); //not used
    rigidBody = new btRigidBody(rigidBodyCI);
    rigidBody->setCollisionFlags(rigidBody->getCollisionFlags() | btCollisionObject::CF_STATIC_OBJECT | btCollisionObject::CF_CUSTOM_MATERIAL_CALLBACK);
    
    //rendering mesh
    GLfloat offsetX = ((width-1)/2.0)*size;
    GLfloat offsetZ = ((length-1)/2.0)*size;
    GLfloat offsetY = (maxHeight-minHeight)/2.0;

    displayList = glGenLists(1);
    glNewList(displayList, GL_COMPILE);
    for(int i=0; i<length-1; i++)
    {
        glBegin(GL_TRIANGLE_STRIP);
        for(int h=0; h<width; h++)
        {
            glNormal3f(terrainNormals[i*width+h].x(), terrainNormals[i*width+h].y(), terrainNormals[i*width+h].z());
            glVertex3f(h*size - offsetX, terrainHeight[i*width+h] - offsetY, (i)*size - offsetZ);
            glNormal3f(terrainNormals[(i+1)*width+h].x(), terrainNormals[(i+1)*width+h].y(), terrainNormals[(i+1)*width+h].z());
            glVertex3f(h*size - offsetX, terrainHeight[(i+1)*width+h] - offsetY, (i+1)*size - offsetZ);
        }
        glEnd();
    }
    glEndList();
    
    delete [] terrainNormals;
}

TerrainEntity::~TerrainEntity()
{
    if(displayList != 0)
        glDeleteLists(displayList, 1);
    if(look.texture != 0)
        glDeleteTextures(1, &look.texture);
    
    material = NULL;
    rigidBody = NULL;
}

EntityType TerrainEntity::getType()
{
    return TERRAIN;
}

void TerrainEntity::Render()
{
    if(isRenderable())
    {
        btTransform trans;
        btScalar openglTrans[16];
        rigidBody->getMotionState()->getWorldTransform(trans);
        trans.getOpenGLMatrix(openglTrans);
        
        glPushMatrix();
        glPushAttrib(GL_POLYGON_BIT);
#ifdef BT_USE_DOUBLE_PRECISION
        glMultMatrixd(openglTrans);
#else
        glMultMatrixf(openglTrans);
#endif
        UseLook(look);
        if(wireframe)
            glPolygonMode(GL_FRONT, GL_LINE);
        glCallList(displayList);
        glPopAttrib();
        glPopMatrix();
    }
}

btTransform TerrainEntity::getTransform()
{
    btTransform trans;
    rigidBody->getMotionState()->getWorldTransform(trans);
    return trans;
}

void TerrainEntity::setTransform(const btTransform &trans)
{
    btDefaultMotionState* motionState = new btDefaultMotionState(trans);
    rigidBody->setMotionState(motionState);
}

Material* TerrainEntity::getMaterial()
{
    return material;
}

void TerrainEntity::AddToDynamicsWorld(btDynamicsWorld *world)
{
    world->addRigidBody(rigidBody, DEFAULT, DEFAULT | CABLE_EVEN | CABLE_ODD);
}

void TerrainEntity::SetLook(Look newLook)
{
    if(look.texture != 0)
        glDeleteTextures(1, &look.texture);
    
    look = newLook;
}

void TerrainEntity::setDrawWireframe(bool wire)
{
    wireframe = wire;
}






/*
////////////////
static btScalar randomHeight(int step, int gridSize, btScalar gridSpacing)
{
	return (0.33 * gridSpacing * gridSize * step * (rand() - (0.5 * RAND_MAX))) / (1.0 * RAND_MAX * gridSize);
}

// creates a random, fractal heightfield
static void setFractal(btScalar *grid, int step, int gridSize, btScalar gridSpacing)
{
	int newStep = step / 2;
    
	// special case: starting (must set four corners)
	if (gridSize - 1 == step) {
		// pick a non-zero (possibly negative) base elevation for testing
		btScalar base = randomHeight(step/2, gridSpacing, gridSize);
        
		convertFromFloat(grid, base, type);
		convertFromFloat(grid + step * bytesPerElement, base, type);
		convertFromFloat(grid + step * s_gridSize * bytesPerElement, base, type);
		convertFromFloat(grid + (step * s_gridSize + step) * bytesPerElement, base, type);
	}
    
	// determine elevation of each corner
	float c00 = convertToFloat(grid, type);
	float c01 = convertToFloat(grid + step * bytesPerElement, type);
	float c10 = convertToFloat(grid + (step * s_gridSize) * bytesPerElement, type);
	float c11 = convertToFloat(grid + (step * s_gridSize + step) * bytesPerElement, type);
    
	// set top middle
	updateHeight(grid + newStep * bytesPerElement, 0.5 * (c00 + c01) + randomHeight(step), type);
    
	// set left middle
	updateHeight(grid + (newStep * s_gridSize) * bytesPerElement, 0.5 * (c00 + c10) + randomHeight(step), type);
    
	// set right middle
	updateHeight(grid + (newStep * s_gridSize + step) * bytesPerElement, 0.5 * (c01 + c11) + randomHeight(step), type);
    
	// set bottom middle
	updateHeight(grid + (step * s_gridSize + newStep) * bytesPerElement, 0.5 * (c10 + c11) + randomHeight(step), type);
    
	// set middle
	updateHeight(grid + (newStep * s_gridSize + newStep) * bytesPerElement, 0.25 * (c00 + c01 + c10 + c11) + randomHeight(step), type);
    
    //	std::cerr << "Computing grid with step = " << step << ": after\n";
    //	dumpGrid(grid, bytesPerElement, type, step + 1);
    
	// terminate?
	if (newStep < 2) {
		return;
	}
    
	// recurse
	setFractal(grid, bytesPerElement, type, newStep);
	setFractal(grid + newStep * bytesPerElement, bytesPerElement, type, newStep);
	setFractal(grid + (newStep * s_gridSize) * bytesPerElement, bytesPerElement, type, newStep);
	setFractal(grid + ((newStep * s_gridSize) + newStep) * bytesPerElement, bytesPerElement, type, newStep);
}
*/

