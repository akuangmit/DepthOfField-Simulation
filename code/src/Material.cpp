#include "Material.h"

Vector3f Material::shade(const Ray &ray,
    const Hit &hit,
    const Vector3f &dirToLight,
    const Vector3f &lightIntensity)
{
	Vector3f finalIntensity(0,0,0);

	// Diffuse phong terms
    Vector3f norm = hit.getNormal();
    float diffShading = Vector3f::dot(norm, dirToLight);
    if (diffShading < 0) {
    	diffShading = 0;
    }

    for (int i = 0; i < 3; i++) {
    	finalIntensity[i] += diffShading * lightIntensity[i] * _diffuseColor[i];
    }

    // specular phong terms
    float s = hit.getMaterial()->_shininess;

    Vector3f eyeDir = - ray.getDirection();

    Vector3f eyeReflection = - eyeDir + 2 * (Vector3f::dot(eyeDir, norm)) * norm;

    float specularShading = Vector3f::dot(eyeReflection, dirToLight);
    if (specularShading < 0) {
    	specularShading = 0;
    }

    for (int i = 0; i < 3; i++) {
    	finalIntensity[i] += pow(specularShading, s) * lightIntensity[i] * _specularColor[i];
    }

    return finalIntensity;
}
