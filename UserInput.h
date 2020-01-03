#pragma once


//Manage the users input via keyboard
class Vector3 {
public:
	float x;
	float y;
	float z;
};



class UserInput
{
public:
	float xPos;
	float yPos;
	float zPos;
	Vector3 vecMin;
	Vector3 vecMax;
	float rotationX;
	float rotationY;
	float rotationZ;
	bool isMoving;
	bool isMovingRight;
	bool isMovingLeft;
	bool isMovingUp;
	bool isMovingDown;
	bool isMovingCloser;
	bool isMovingFurther;
	bool isRotatingX;
	bool isRotatingY;
	bool isRotatingZ;
	UserInput()
	{
		rotationX = 0.0f;
		rotationY = 0.0f;
		rotationZ = 0.0f;
		isMovingRight = false;
		isMovingLeft = false;
		isMovingUp = false;
		isMovingDown = false;
		isMovingCloser = false;
		isMovingFurther = false;
		isRotatingX = false;
		isRotatingY = false;
		isRotatingZ = false;
	}

	void UpdatePos()
	{
		//-1 and +1 from the origin of the cube (centre)
		vecMin.x = xPos - 1.0f;
		vecMax.x = xPos + 1.0f;
		vecMin.y = yPos - 1.0f;
		vecMax.y = yPos + 1.0f;
		vecMin.z = zPos - 1.0f;
		vecMax.z = zPos + 1.0f;
	}
};

