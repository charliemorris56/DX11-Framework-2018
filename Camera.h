#pragma once

#include <windows.h>
#include <d3d11_1.h>
#include <d3dcompiler.h>
#include <directxmath.h>
#include <directxcolors.h>

using namespace DirectX;

class Camera
{
private:
	XMFLOAT3 _eye;
	XMFLOAT3 _at;
	XMFLOAT3 _up;

	FLOAT _windowWidth;
	FLOAT _windowHeight;
	FLOAT _nearDepth;
	FLOAT _farDepth;

	XMFLOAT4X4 _view;
	XMFLOAT4X4 _projection;

public:
	Camera(XMFLOAT3 position, XMFLOAT3 at, XMFLOAT3 up, FLOAT windowWidth, FLOAT windowHeight, FLOAT nearDepth, FLOAT farDepth);
	~Camera();

	void update();

	XMFLOAT3 getEye();
	XMFLOAT3 getAt();
	XMFLOAT3 getUp();
	void setEye(XMFLOAT3 position);
	void setAt(XMFLOAT3 at);
	void setUp(XMFLOAT3 up);

	XMFLOAT4X4 getView();
	XMFLOAT4X4 getProjection();
	XMFLOAT4X4 getViewProjection();

	void Reshape(FLOAT widowWidth, FLOAT windowHeight, FLOAT nearDepth, FLOAT farDepth);
};