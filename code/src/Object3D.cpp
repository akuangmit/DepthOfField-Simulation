#include "Object3D.h"
#include <iostream>

bool Sphere::intersect(const Ray &r, float tmin, Hit &h) const
{
    // BEGIN STARTER

    // We provide sphere intersection code for you.
    // You should model other intersection implementations after this one.

    // Locate intersection point ( 2 pts )
    const Vector3f &rayOrigin = r.getOrigin(); //Ray origin in the world coordinate
    const Vector3f &dir = r.getDirection();

    Vector3f origin = rayOrigin - _center;      //Ray origin in the sphere coordinate

    float a = dir.absSquared();
    float b = 2 * Vector3f::dot(dir, origin);
    float c = origin.absSquared() - _radius * _radius;

    // no intersection
    if (b * b - 4 * a * c < 0) {
        return false;
    }

    float d = sqrt(b * b - 4 * a * c);

    float tplus = (-b + d) / (2.0f*a);
    float tminus = (-b - d) / (2.0f*a);

    // the two intersections are at the camera back
    if ((tplus < tmin) && (tminus < tmin)) {
        return false;
    }

    float t = 10000;
    // the two intersections are at the camera front
    if (tminus > tmin) {
        t = tminus;
    }

    // one intersection at the front. one at the back 
    if ((tplus > tmin) && (tminus < tmin)) {
        t = tplus;
    }

    if (t < h.getT()) {
        Vector3f normal = r.pointAtParameter(t) - _center;
        normal = normal.normalized();
        h.set(t, this->material, normal);
        return true;
    }
    // END STARTER
    return false;
}

bool Cone::intersect(const Ray &r, float tmin, Hit &h) const
{
    const Vector3f &rayOrigin = r.getOrigin(); //Ray origin in the world coordinate
    const Vector3f &dir = r.getDirection().normalized();

    const Vector3f &axis = - _axis.normalized(); // pointing "downwards"
    const Vector3f &tip = _center + (-axis) * _height;
    const float costheta2 = pow(_height / sqrt(_height * _height + _radius * _radius), 2);
    const Vector3f &tip2o = rayOrigin - tip;

    float a = pow(Vector3f::dot(dir, axis),2) - costheta2;
    float b = 2 * (Vector3f::dot(dir, axis) * Vector3f::dot(tip2o, axis) - Vector3f::dot(dir, tip2o)*costheta2);
    float c = pow(Vector3f::dot(tip2o, axis), 2) - Vector3f::dot(tip2o, tip2o) * costheta2;

    // no intersection
    if (b * b - 4 * a * c < 0) {
        return false;
    }

    float d = sqrt(b * b - 4 * a * c);

    float tplus = (-b + d) / (2.0f*a);
    float tminus = (-b - d) / (2.0f*a);

    // the two intersections are at the camera back
    if ((tplus < tmin) && (tminus < tmin)) {
        return false;
    }

    // shadow intersections
    if (Vector3f::dot(r.pointAtParameter(tplus) - tip, axis) <= 0 && Vector3f::dot(r.pointAtParameter(tminus) - tip, axis) <= 0) {
        return false;
    }

    float t = 10000;
    // the two intersections are at the camera front
    if (tminus > tmin) {
        t = tminus;
    }

    // one intersection at the front. one at the back 
    if ((tplus > tmin) && (tminus < tmin)) {
        t = tplus;
    }

    if (t < h.getT()) {
        Vector3f normal = r.pointAtParameter(t) - _center;
        normal = normal.normalized();
        h.set(t, this->material, normal);
        return true;
    }
    // END STARTER
    return false;
}

// Add object to group
void Group::addObject(Object3D *obj) {
    m_members.push_back(obj);
}

// Return number of objects in group
int Group::getGroupSize() const {
    return (int)m_members.size();
}

bool Group::intersect(const Ray &r, float tmin, Hit &h) const
{
    // BEGIN STARTER
    // we implemented this for you
    bool hit = false;

    for (Object3D* o : m_members) {
        if (o->intersect(r, tmin, h)) {
            hit = true;
        }
    }
    return hit;
    // END STARTER
}

bool Plane::intersect(const Ray &r, float tmin, Hit &h) const
{
    const Vector3f &rayOrigin = r.getOrigin(); //Ray origin in the world coordinate
    const Vector3f &dir = r.getDirection();

    float denom = Vector3f::dot(_normal, dir);

    // ray looks directly at plane, so no intersection
    if (denom == 0) {
        return false;
    }

    float t = -(-_d + Vector3f::dot(_normal, rayOrigin)) / denom;

    // intersection too close
    if (t <= tmin) {
        return false;
    }

    if (t < h.getT()) {
        h.set(t, this->material, _normal.normalized());
        return true;
    }

    return false;
}
bool Triangle::intersect(const Ray &r, float tmin, Hit &h) const 
{
    const Vector3f &rayOrigin = r.getOrigin();
    const Vector3f &dir = r.getDirection();
    Matrix3f mat(_v[0][0] - _v[1][0], _v[0][0] - _v[2][0], dir[0],
                 _v[0][1] - _v[1][1], _v[0][1] - _v[2][1], dir[1],
                 _v[0][2] - _v[1][2], _v[0][2] - _v[2][2], dir[2]);
    Vector3f sol(_v[0][0] - rayOrigin[0], _v[0][1] - rayOrigin[1], _v[0][2] - rayOrigin[2]);

    Vector3f ans = mat.inverse() * sol;
    float beta = ans[0];
    float gamma = ans[1];
    float t = ans[2];

    // did not intersect with triangle
    if (beta < 0 || gamma < 0 || (1 - beta - gamma < 0)) {
        return false;
    }

    // Vector3f v1 = _v[1] - _v[0];
    // Vector3f v2 = _v[2] - _v[0];
    // Vector3f normal = Vector3f::cross(v1, v2);

    Vector3f normal = (1-beta-gamma) * _normals[0] + beta * _normals[1] + gamma * _normals[2]; 

    if (t <= tmin) {
        return false;
    }

    if (t < h.getT()) {
        h.set(t, this->material, normal.normalized());
        return true;
    }

    return false;
}

bool Transform::intersect(const Ray &r, float tmin, Hit &h) const
{
    Matrix4f inv = _m.inverse();
    Vector4f rayOrigin(r.getOrigin(), 1);
    Vector4f dir(r.getDirection(), 0);
    Ray newR((inv * rayOrigin).xyz(), (inv * dir).xyz());

    if (_object->intersect(newR, tmin, h)) {
        Vector4f norm(h.getNormal(), 0);
        Vector3f newN = (inv.transposed() * norm).xyz();
        h.set(h.getT(), _object->material, newN.normalized());

        return true;
    }
    return false;
}
