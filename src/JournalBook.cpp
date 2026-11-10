#include "JournalBook.h"
#include "resourceManager.h"
#include "shaderSetup.h"

namespace
{
    RenderTexture2D* bookTexture = nullptr;
}

void InitJournalBook()
{
    if (bookTexture != nullptr)
    {
        return;
    }

    bookTexture = &ResourceManager::Get().LoadRenderTexture(
        "journalBook",
        GetScreenWidth(),
        GetScreenHeight()
    );
}

bool DrawJournalBook(JournalBookLayout& layout)
{
    if (bookTexture == nullptr)
    {
        return false;
    }

    // Move the original book drawing block here.
    // It starts at Texture2D& paper = R.GetTexture("paper");
    // and ends after the closing }; of Rectangle rightPageScreen.

    Texture2D& paper = R.GetTexture("paper");
    Texture2D& borderLeft  = R.GetTexture("borderLeft");
    Texture2D& borderRight = R.GetTexture("borderRight");

    float pageWidth = 550.0f;
    float pageHeight = 650.0f;
    float centerX = bookTexture->texture.width / 2.0f;
    float centerY = bookTexture->texture.height / 2.0f;
    float spineGap = 2.0f;
    float spineBackingWidth = 8.0f;
    float spineGradientWidth = 32.0f;

    float leftSpineGradientWidth = 16.0f;
    float rightSpineGradientWidth = 32.0f;

    Rectangle source{
        0.0f,
        0.0f,
        static_cast<float>(paper.width),
        static_cast<float>(paper.height)
    };

    Rectangle leftPage{
        centerX - pageWidth - spineGap / 2.0f,
        centerY - pageHeight / 2.0f,
        pageWidth,
        pageHeight
    };

    Rectangle rightPage{
        centerX + spineGap / 2.0f,
        centerY - pageHeight / 2.0f,
        pageWidth,
        pageHeight
    };



    BeginTextureMode(*bookTexture);
    {
        ClearBackground(BLANK);

        // Backing beneath the transparent inner edges near the gutter.
        DrawRectangle(
            static_cast<int>(centerX - spineBackingWidth / 2.0f),
            static_cast<int>(centerY - pageHeight / 2.0f),
            static_cast<int>(spineBackingWidth),
            static_cast<int>(pageHeight),
            BLACK
        );

        // Shadows cast by the page stacks onto the book cover.
        DrawRectangleRounded(
            Rectangle{
                leftPage.x - 7.0f,
                leftPage.y + 11.0f,
                leftPage.width,
                leftPage.height
            },
            0.025f,
            8,
            Color{30, 18, 8, 130}
        );

        DrawRectangleRounded(
            Rectangle{
                rightPage.x + 7.0f,
                rightPage.y + 11.0f,
                rightPage.width,
                rightPage.height
            },
            0.025f,
            8,
            Color{30, 18, 8, 130}
        );

        // Lower, darker sheets.
        DrawRectangleRounded(
            Rectangle{
                leftPage.x - 8.0f,
                leftPage.y - 2.0f,
                leftPage.width + 10.0f,
                leftPage.height + 12.0f
            },
            0.025f,
            8,
            Color{135, 108, 68, 255}
        );

        DrawRectangleRounded(
            Rectangle{
                rightPage.x - 2.0f,
                rightPage.y - 2.0f,
                rightPage.width + 10.0f,
                rightPage.height + 12.0f
            },
            0.025f,
            8,
            Color{135, 108, 68, 255}
        );

        // Upper sheets, closer to the visible pages.
        DrawRectangleRounded(
            Rectangle{
                leftPage.x - 4.0f,
                leftPage.y - 1.0f,
                leftPage.width + 5.0f,
                leftPage.height + 6.0f
            },
            0.025f,
            8,
            Color{185, 158, 106, 255}
        );

        DrawRectangleRounded(
            Rectangle{
                rightPage.x - 1.0f,
                rightPage.y - 1.0f,
                rightPage.width + 5.0f,
                rightPage.height + 6.0f
            },
            0.025f,
            8,
            Color{185, 158, 106, 255}
        );

        const float stackBottom =
            leftPage.y + leftPage.height + 10.0f;

        const float innerRise = 9.0f;
        const float angleWidth = 100.0f;

        Color stackShade{75, 48, 25, 150};

        // Left stack darkens and rises toward the spine.
        DrawTriangle(
            Vector2{centerX, stackBottom - innerRise},
            Vector2{centerX, stackBottom},
            Vector2{centerX - angleWidth, stackBottom},
            stackShade
        );

        // Right stack mirrors it.
        DrawTriangle(
            Vector2{centerX, stackBottom - innerRise},
            Vector2{centerX + angleWidth, stackBottom},
            Vector2{centerX, stackBottom},
            stackShade
        );

        // Main visible pages.
        DrawTexturePro(paper, source, leftPage, {0.0f, 0.0f}, 0.0f, WHITE);
        DrawTexturePro(paper, source, rightPage, {0.0f, 0.0f}, 0.0f, WHITE);

        // Decorative upper outside corners.



        Rectangle borderLeftSource{
            0.0f,
            0.0f,
            static_cast<float>(borderLeft.width),
            static_cast<float>(borderLeft.height)
        };

        Rectangle borderRightSource{
            0.0f,
            0.0f,
            static_cast<float>(borderRight.width),
            static_cast<float>(borderRight.height)
        };

        const float borderSize = 400.0f;
        const float borderInset  = 0.0f;


        Rectangle borderLeftDestination{
            leftPage.x + borderInset,
            leftPage.y + borderInset,
            borderSize,
            borderSize
        };

        Rectangle borderRightDestination{
            rightPage.x + rightPage.width - borderSize - borderInset,
            rightPage.y + borderInset,
            borderSize,
            borderSize
        };

        Color borderTint{92, 61, 34, 150};

        DrawTexturePro(
            borderLeft,
            borderLeftSource,
            borderLeftDestination,
            {0.0f, 0.0f},
            0.0f,
            borderTint
        );

        DrawTexturePro(
            borderRight,
            borderRightSource,
            borderRightDestination,
            {0.0f, 0.0f},
            0.0f,
            borderTint
        );


        // Visible center crease drawn over both pages.
        float visibleSpineWidth = 3.0f;

        DrawRectangle(
            static_cast<int>(centerX - visibleSpineWidth / 2.0f),
            static_cast<int>(centerY - pageHeight / 2.0f),
            static_cast<int>(visibleSpineWidth),
            static_cast<int>(pageHeight),
            Color{18, 8, 5, 220}
        );

        float leftInnerEdge = leftPage.x + leftPage.width;
        float rightInnerEdge = rightPage.x;

        Color transparent{0, 0, 0, 0};
        Color spineShadow{70, 50, 20, 150};

        // Left-page gutter shadow.
        DrawRectangleGradientH(
            static_cast<int>(leftInnerEdge - leftSpineGradientWidth),
            static_cast<int>(leftPage.y),
            static_cast<int>(leftSpineGradientWidth),
            static_cast<int>(leftPage.height),
            transparent,
            spineShadow
        );

        // Right-page gutter shadow.
        DrawRectangleGradientH(
            static_cast<int>(rightInnerEdge),
            static_cast<int>(rightPage.y),
            static_cast<int>(rightSpineGradientWidth),
            static_cast<int>(rightPage.height),
            spineShadow,
            transparent
        );
    }
    EndTextureMode();



    Rectangle bookSource{
        0.0f,
        0.0f,
        static_cast<float>(bookTexture->texture.width),
        -static_cast<float>(bookTexture->texture.height)
    };

    Rectangle bookDestination{
        0.0f,
        0.0f,
        static_cast<float>(GetScreenWidth()),
        static_cast<float>(GetScreenHeight())
    };

    float scaleX =
        static_cast<float>(GetScreenWidth()) /
        static_cast<float>(bookTexture->texture.width);

    float scaleY =
        static_cast<float>(GetScreenHeight()) /
        static_cast<float>(bookTexture->texture.height);

    float coverBorder = 22.0f;

    // Page spread bounds converted from render-texture space
    // into screen space.
    Rectangle coverScreen{
        (leftPage.x - coverBorder) * scaleX,
        (leftPage.y - coverBorder) * scaleY,

        (rightPage.x + rightPage.width -
            leftPage.x + coverBorder * 2.0f) * scaleX,

        (leftPage.height + coverBorder * 2.0f) * scaleY
    };

    Rectangle spineBackingScreen{
        (centerX - spineBackingWidth / 2.0f) * scaleX,
        coverScreen.y,
        spineBackingWidth * scaleX,
        coverScreen.height
    };

    // Main leather cover behind the pages.
    const float coverRoundness = 0.035f;
    const int coverSegments = 12;

    DrawRectangleRounded(
        coverScreen,
        coverRoundness,
        coverSegments,
        Color{55, 25, 16, 255}
    );


    // Dark center binding extending across the entire cover.
    DrawRectangleRec(
        spineBackingScreen,
        Color{18, 8, 5, 255}
    );

    float screenGradientWidth = spineGradientWidth * scaleX;
    float spineCenterScreenX = centerX * scaleX;

    // Left side of the cover's spine shadow.
    DrawRectangleGradientH(
        static_cast<int>(
            spineCenterScreenX - screenGradientWidth
        ),
        static_cast<int>(coverScreen.y),
        static_cast<int>(screenGradientWidth),
        static_cast<int>(coverScreen.height),
        Color{0, 0, 0, 0},
        Color{18, 8, 5, 255}
    );

    // Right side of the cover's spine shadow.
    DrawRectangleGradientH(
        static_cast<int>(spineCenterScreenX),
        static_cast<int>(coverScreen.y),
        static_cast<int>(screenGradientWidth),
        static_cast<int>(coverScreen.height),
        Color{18, 8, 5, 255},
        Color{0, 0, 0, 0}
    );

    BeginShaderMode(ShaderSetup::gJournal.shader);
    DrawTexturePro(
        bookTexture->texture,
        bookSource,
        bookDestination,
        Vector2{0.0f, 0.0f},
        0.0f,
        WHITE
    );
    EndShaderMode();

    Rectangle leftPageScreen{
        leftPage.x * scaleX,
        leftPage.y * scaleY,
        leftPage.width * scaleX,
        leftPage.height * scaleY
    };

    Rectangle rightPageScreen{
        rightPage.x * scaleX,
        rightPage.y * scaleY,
        rightPage.width * scaleX,
        rightPage.height * scaleY
    };

    layout = {leftPageScreen, rightPageScreen, scaleX, scaleY};
    return true;
}