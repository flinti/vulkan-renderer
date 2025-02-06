#include "Camera.h"

#include <algorithm>
#include <cassert>
#include <cmath>
#include <glm/ext/matrix_transform.hpp>
#include <glm/ext/scalar_constants.hpp>
#include <glm/ext/vector_float3.hpp>
#include <glm/geometric.hpp>
#include <glm/gtc/constants.hpp>
#include <glm/trigonometric.hpp>
#include <limits>
#include "../third-party/spdlog/include/spdlog/spdlog.h"
#include <vulkan/vulkan.h>

Camera::Camera()
    : fovy(glm::radians(45.f)),
    aspect(1.f),
    zNear(0.1f),
    zFar(100.f),
    center(0.f),
    eye(0.f, 0.f, 1.f),
    r(1.f),
    theta(0.f),
    phi(0.f),
    currentTransform(1.f),
    v(1.f),
    p(perspective(fovy, aspect, zNear, zFar))
{
    lookAt(center, r, theta, phi);
}

Camera::Camera(float fovy, float aspect, float zNear, float zFar)
    : fovy(fovy),
    aspect(aspect),
    zNear(zNear),
    zFar(zFar),
    center(0.f),
    eye(0.f, 0.f, 1.f),
    r(1.f),
    theta(0.f),
    phi(0.f),
    currentTransform(1.f),
    v(1.f),
    p(perspective(fovy, aspect, zNear, zFar))
{
    lookAt(center, r, theta, phi);
}

Camera::~Camera()
{
}


glm::mat4 Camera::getTransform() const
{
    return currentTransform;
}

glm::vec3 Camera::getEye() const
{
    return eye;
}

void Camera::lookAt(glm::vec3 center, float r, float theta, float phi)
{
    this->center = center;
    this->r = r;
    this->theta = theta;
    this->phi = phi;

	calcView(center, r, theta, phi);
    updateTransform();
}


void Camera::drag(float dx, float dy)
{
    float dphi = std::atan(dx / r);
    float dtheta = std::atan(dy / r);

    float epsilon = 0.000001f;
    theta = std::clamp(theta + dtheta, epsilon, glm::pi<float>() - epsilon);
    phi = std::fmod(phi + dphi, glm::two_pi<float>());
    calcView(center, r, theta, phi);
    updateTransform();
}

void Camera::setAspect(float aspect)
{
    this->aspect = aspect;
    p = perspective(fovy, aspect, zNear, zFar);
    updateTransform();
}

void Camera::setRadius(float radius)
{
    r = std::max(radius, 0.f);
    calcView(center, r, theta, phi);
    updateTransform();
}

void Camera::addRadius(float dr)
{
    setRadius(r + dr);
}


void Camera::addFar(float dzFar)
{
    this->zFar = std::clamp(this->zFar + dzFar, 1.f, 10000.f);
    p = perspective(fovy, aspect, zNear, zFar);
    updateTransform();
}

void Camera::setCenter(glm::vec3 center)
{
    this->center = center;
    calcView(center, r, theta, phi);
    updateTransform();
}

void Camera::moveCenter(glm::vec3 dc)
{
    setCenter(center + dc);
}

void Camera::updateTransform()
{
    currentTransform = p * v;
}

glm::mat4 Camera::perspective(float fovy, float aspect, float zNear, float zFar)
{
    assert(std::abs(aspect - std::numeric_limits<float>::epsilon()) > 0.f);

    float const tanHalfFovy = std::tan(fovy / 2.f);

    glm::mat4 result{0.f};
    result[0][0] = 1.f / (aspect * tanHalfFovy);
    result[1][1] = -1.f / (tanHalfFovy);
    result[2][2] = zFar / (zNear - zFar);
    result[2][3] = -1.f;
    result[3][2] = -(zFar * zNear) / (zFar - zNear);
    return result;
}


void Camera::calcView(glm::vec3 center, float r, float theta, float phi)
{
    float x = r * std::sin(theta) * std::cos(phi);
	float y = r * std::sin(theta) * std::sin(phi);
	float z = r * std::cos(theta);

    eye = glm::vec3(x, y, z);
    v = glm::lookAt(eye, center, glm::vec3(0.f, 0.f, 1.f));
}
