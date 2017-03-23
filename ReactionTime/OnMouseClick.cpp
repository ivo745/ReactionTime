#include "pch.h"
#include "Game.h"
#include "Buttons.h"
#include "FileHandler.h"

using namespace DirectX::SimpleMath;

float calculateSign(Vector2 mPoint, Vector2 v1, Vector2 v2)
{
	return (mPoint.x - v2.x) * (v1.y - v2.y) - (v1.x - v2.x) * (mPoint.y - v2.y);
}

bool isCursorInsideTriangle(Vector2 mPoint, Vector2 v1, Vector2 v2, Vector2 v3)
{
	bool b1, b2, b3;

	b1 = calculateSign(mPoint, v1, v2) < 0.0f;
	b2 = calculateSign(mPoint, v2, v3) < 0.0f;
	b3 = calculateSign(mPoint, v3, v1) < 0.0f;

	return ((b1 == b2) && (b2 == b3));
}

bool isCursorInsideRectangle(Vector2 mPoint, Vector2 v1, Vector2 v2, Vector2 v3, Vector2 v4)
{
	bool b1, b2, b3, b4;

	b1 = calculateSign(mPoint, v1, v2) < 0.0f;
	b2 = calculateSign(mPoint, v2, v3) < 0.0f;
	b3 = calculateSign(mPoint, v3, v4) < 0.0f;
	b4 = calculateSign(mPoint, v4, v1) < 0.0f;

	return ((b1 == b2) && (b2 == b3) && (b3 == b4));
}

Vector2 Game::mPoint()
{
	POINT p;
	if (GetCursorPos(&p))
	{
		if (ScreenToClient(m_window, &p))
			return Vector2((float)p.x, (float)p.y);
	}
	return Vector2(0.0f, 0.0f);
}

bool Game::isCursorInsideUnlock()
{
	Vector2 v1(305.0f + 125.0f, 385.0f - 50.0f);
	Vector2 v2(305.0f + 75.0f, 385.0f);
	Vector2 v3(160.0f, 385.0f);
	Vector2 v4(160.0f, 385.0f - 50.0f);
	if (isCursorInsideRectangle(mPoint(), v1, v2, v3, v4))
		return true;
	return false;
}

bool Game::IsCursorInsideShape()
{
	if (!useOwnShape)
	{
		switch (randShape)
		{
		    case shape_triangle:
		    {
		    	Vector2 v1(t.x + t.r, t.y - ShapeSize() + t.r);
		    	Vector2 v2(t.x, t.y + t.r);
		    	Vector2 v3(t.x - ShapeSize() + t.r, t.y - t.r);
		    	if (isCursorInsideTriangle(mPoint(), v1, v2, v3))
		    		return true;
		    	break;
		    }
		    case shape_rectangle:
		    {
		    	Vector2 v1(r.x, r.y - ShapeSize() + r.r);
		    	Vector2 v2(r.x - r.r, r.y);
		    	Vector2 v3(r.x - ShapeSize(), r.y - r.r);
		    	Vector2 v4(r.x - ShapeSize() + r.r, r.y - ShapeSize());
		    	if (isCursorInsideRectangle(mPoint(), v1, v2, v3, v4))
		    		return true;
		    	break;
		    }
		}
	}
	else
	{
		if (drawShape)
		{
			for (int i = 0; i < ownButtonShape; i++)
			{
				Vector2 v1(MousePoint[0].x + ownShape.x, MousePoint[0].y + ownShape.y);
				Vector2 v2(MousePoint[i + 1].x + ownShape.x, MousePoint[i + 1].y + ownShape.y);
				Vector2 v3(MousePoint[i + 2].x + ownShape.x, MousePoint[i + 2].y + ownShape.y);
				if (isCursorInsideTriangle(mPoint(), v1, v2, v3))
					return true;
			}
		}
	}
	return false;
}

bool Game::IsCursorInsideButton(ButtonTag tag)
{
	if (isCursorInsideRectangle(mPoint(), ButtonArray[tag][0], ButtonArray[tag][1], ButtonArray[tag][2], ButtonArray[tag][3]))
		return true;
	return false;
}
