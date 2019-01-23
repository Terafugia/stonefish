//
//  Jet.cpp
//  Stonefish
//
//  Created by Patryk Cieslak on 10/07/18.
//  Copyright(c) 2018-2019 Patryk Cieslak. All rights reserved.
//

#include "entities/forcefields/Jet.h"

namespace sf
{

Jet::Jet(const Vector3& point, const Vector3& direction, Scalar radius, Scalar outletVelocity)
{
    c = point;
    n = direction.normalized();
    r = radius;
    vout = outletVelocity;
}

Vector3 Jet::GetVelocityAtPoint(const Vector3& p)
{
    //Calculate distance to axis
    Vector3 cp = p-c;
    Scalar d = cp.cross(n).norm();
    
    //Calculate distance from outlet
    Scalar t = cp.dot(n);
    if(t < 0.0) return Vector3(0,0,0);
    
    //Calculate radius at point
    Scalar r_ = Scalar(1)/Scalar(5)*(t + Scalar(5)*r); //Jet angle is around 24 deg independent of conditions!
    if(d >= r_) return Vector3(0,0,0);
    
    //Calculate central velocity
    Vector3 vmax = Scalar(10)*r/(t + Scalar(5)*r) * vout * n;  
    
    //Calculate fraction of central velocity
    Scalar f = btExp(-Scalar(50)*d*d/(t*t));
    
    return f*vmax;
}

std::vector<Renderable> Jet::Render()
{
    std::vector<Renderable> items(0);
    
    //Model matrix
    glm::vec3 z_(n.x(), n.y(), n.z());
    glm::vec3 x_(n.y(), n.x(), n.z());
    glm::vec3 y_ = glm::cross(z_,x_);
    glm::mat4 model(glm::vec4(x_, 0.f), glm::vec4(y_, 0.f), glm::vec4(z_, 0.f), glm::vec4(c.x(), c.y(), c.z(), 1));    
    
    //Orifice
    Renderable orifice;
    orifice.type = RenderableType::HYDRO_LINE_STRIP;
    orifice.model = model;
    
    for(unsigned int i=0; i<=12; ++i)
    {
        Scalar alpha = Scalar(i)/Scalar(12) * M_PI * Scalar(2);
        Vector3 v(btCos(alpha)*r, btSin(alpha)*r, 0);
        orifice.points.push_back(glm::vec3(v.x(), v.y(), v.z()));
    }
    
    //Cone
    Renderable cone;
    cone.type = RenderableType::HYDRO_LINES;
    cone.model = orifice.model;
    cone.points.push_back(glm::vec3(0, 0, 0));
    cone.points.push_back(glm::vec3(0, 0, vout));
    
    Scalar r_ = Scalar(1)/Scalar(5)*(vout + Scalar(5)*r);
    
    for(unsigned int i=0; i<12; ++i)
    {
        Scalar alpha = Scalar(i)/Scalar(12) * M_PI * Scalar(2);
        Vector3 v1(btCos(alpha)*r, btSin(alpha)*r, 0);
        Vector3 v2(v1.x()*r_/r, v1.y()*r_/r, vout);
        cone.points.push_back(glm::vec3(v1.x(), v1.y(), v1.z()));
        cone.points.push_back(glm::vec3(v2.x(), v2.y(), v2.z()));
    }
    
    //Build
    items.push_back(orifice);
    items.push_back(cone);

    return items;
}

}
