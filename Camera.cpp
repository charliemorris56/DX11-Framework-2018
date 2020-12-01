#include "Camera.h"

Camera::Camera(XMFLOAT3 position, XMFLOAT3 at, XMFLOAT3 up, FLOAT windowWidth, FLOAT windowHeight, FLOAT nearDepth, FLOAT farDepth)
{
	_eye = position;
	_at = at;
	_up = up;
	_windowWidth = windowWidth;
	_windowHeight = windowHeight;
	_nearDepth = nearDepth;
	_farDepth = farDepth;
	update();
}

Camera::~Camera()
{
}

void Camera::update()
{
	if (_typeAt)
	{
		XMStoreFloat4x4(&_view, XMMatrixLookAtLH(XMVectorSet(_eye.x, _eye.y, _eye.z, 0.0f), XMVectorSet(_at.x, _at.y, _at.z, 0.0f), XMVectorSet(_up.x, _up.y, _up.z, 0.0f)));
	}
	else
	{
		XMStoreFloat4x4(&_view, XMMatrixLookToLH(XMVectorSet(_eye.x, _eye.y, _eye.z, 0.0f), XMVectorSet(_at.x, _at.y, _at.z, 0.0f), XMVectorSet(_up.x, _up.y, _up.z, 0.0f)));
	}	

	XMStoreFloat4x4(&_projection, XMMatrixPerspectiveFovLH(XM_PIDIV2, _windowWidth / (FLOAT)_windowHeight, 0.01f, 100.0f));
}

XMFLOAT3 Camera::getEye()
{
	return _eye;
}

XMFLOAT3 Camera::getAt()
{
	return _at;
}

XMFLOAT3 Camera::getUp()
{
	return _up;
}

void Camera::setEye(XMFLOAT3 position)
{
	_eye = position;
	update();
}

void Camera::setAt(XMFLOAT3 at)
{
	_at = at;
	update();
}

void Camera::setUp(XMFLOAT3 up)
{
	_up = up;
	update();
}

void Camera::setTypeAt(bool typeAt)
{
	_typeAt = typeAt;
}

XMFLOAT4X4 Camera::getView()
{
	return _view;
}

XMFLOAT4X4 Camera::getProjection()
{
	return _projection;
}

XMFLOAT4X4 Camera::getViewProjection()
{
	return XMFLOAT4X4();
}

bool Camera::getTypeAt()
{
	return _typeAt;
}

void Camera::Reshape(FLOAT windowWidth, FLOAT windowHeight, FLOAT nearDepth, FLOAT farDepth)
{
	_windowWidth = windowWidth;
	_windowHeight = windowHeight;
	_nearDepth = nearDepth;
	_farDepth = farDepth;
}
