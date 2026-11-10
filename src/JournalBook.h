#pragma once

#include "raylib.h"

struct JournalBookLayout
{
    Rectangle leftPageScreen;
    Rectangle rightPageScreen;
    float scaleX;
    float scaleY;
};

void InitJournalBook();
bool DrawJournalBook(JournalBookLayout& layout);