#include "double_jump_task.h"

#include <in_buttons.h>

CBotTFDoubleJump ::CBotTFDoubleJump()
{
	m_fTime = 0.0f;
}

void CBotTFDoubleJump ::execute(CBot *pBot, CBotSchedule *pSchedule)
{
	pBot->wantToListen(false);

	if (m_fTime == 0.0f)
	{
		pBot->tapButton(IN_JUMP);

		m_fTime = engine->Time() + 0.4;
	}
	else if (m_fTime < engine->Time())
	{
		pBot->jump();
		complete();
	}
}

void CBotTFDoubleJump ::debugString(char *string)
{
	sprintf(string, "CbotTFDoublejump");
}