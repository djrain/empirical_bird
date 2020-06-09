#include "web/Animate.h"
#include "web/web.h"
#include "web/KeypressManager.h"
#include "tools/math.h"
#include "base/vector.h"
#include "tools/Random.h"
#include <iostream>

class Bird;
class Game;

emp::web::Document doc("target");
emp::web::KeypressManager keyManager;

const int canvas_width = 480;
const int canvas_height = 720;

//double x = 0;


struct Vec2 {
    double x = 0;
    double y = 0;
};

struct Rect
{
    Vec2 position; // lower left corner
    Vec2 size;
};

bool RectOverlap(Rect r1, Rect r2)
{
	return (r1.position.x + r1.size.x > r2.position.x) && (r1.position.x < r2.position.x + r2.size.x) && (r1.position.y + r1.size.y > r2.position.y) && (r1.position.y < r2.size.y + r2.position.y);
}

class Bird {

public:

    Rect rect;
    Vec2 velocity;

    double gravity = 1700;
    double flapSpeed = 600;
    double maxFallSpeed = 750;

    void Move(double delta, bool gameOver)
    {
        velocity.y = emp::Min(maxFallSpeed, velocity.y + gravity * delta);

        rect.position.x += velocity.x * delta;
        rect.position.y = emp::Max(0.0, rect.position.y + velocity.y * delta);

        if (gameOver)
            rect.position.y = emp::Min(rect.position.y, canvas_height - 60 - rect.size.y*1.04);
    }

    void Draw(emp::web::Canvas canvas)
    {
        canvas.Circle(rect.position.x + rect.size.x/2, rect.position.y + rect.size.y/2, rect.size.x*0.55, "white");
    }

    void Flap()
    {
        velocity.y = -flapSpeed;
    }

};

class Pipe
{

public:

    const static int xSpeed = -240;

    bool spawned = false; //when this pipe passes a certain point on screen, it "spawns" the next pipe by moving it just offscreen.
    bool addedToScore = false;
    Rect rect;
    Vec2 velocity;

    void Move(double delta)
    {
        rect.position.x += velocity.x * delta;
        rect.position.y += velocity.y * delta;
    }

    void Draw(emp::web::Canvas canvas)
    {
        canvas.Rect(rect.position.x, rect.position.y, rect.size.x, rect.size.y, "#1b2336");
    }

    bool ShouldMoveNext(int pipeDistance)
    {
        return (!spawned && rect.position.x < canvas_width - pipeDistance);
    }

    bool UpdateScore()
    {
        if  (!addedToScore && rect.position.x < canvas_width * 0.34 - rect.size.x/2)
        {
            addedToScore = true;
            return true;
        }

        return false;
    }

};


class Cloud
{

}


class Game : public emp::web::Animate {

    // make the canvas
    emp::web::Canvas canvas{canvas_width, canvas_height, "canvas"};
    emp::Random random;
    Bird bird;
    Rect ground{Vec2{0, canvas_height-60}, Vec2{canvas_width, 60}};

    int pipeDistance = 270;
    int pipeHeight = 480;
    int maxOffset = 160;
    int gapSize = 144; // vertical distance between pipes

    Pipe upperPipes[3]; // upper visually, but actually lower y coordinate since canvas 0 is at top.
    Pipe lowerPipes[3];

    int score = 0;
    bool gameOver = false;
    bool startPipes = false; // don't start pipes moving until player has flapped once

public:

    Game()
    {
        canvas.StrokeColor("transparent");
        canvas.Font("34px Comic Sans MS");
        canvas.SetCSS("display", "block");
        canvas.SetCSS("margin-left", "auto");
        canvas.SetCSS("margin-right", "auto");

        // shove canvas into the div
        doc << canvas;

        bird.rect.position.x = canvas_width * 0.4;
        bird.rect.position.y = canvas_height * 0.5;
        bird.rect.size.x = 21;
        bird.rect.size.y = 21;

        for (int i = 0; i < 3; i++)
        {
            upperPipes[i] = Pipe();
            lowerPipes[i] = Pipe();
        }

    }

    // overrides base class's virtual function
    void DoFrame() override
    {
        Update();
        Draw();
    }

    void Update()
    {
        if (!startPipes)
            return;

        double delta = GetStepTime() / 1000; // it's in milliseconds
        bird.Move(delta, gameOver);

        if (gameOver)
            return;

        for (auto& pipe : upperPipes)
            pipe.Move(delta);

        for (auto& pipe : lowerPipes)
            pipe.Move(delta);

        // recycle pipes
        for (int i = 0; i < 3; i++)
        {
            if (upperPipes[i].ShouldMoveNext(pipeDistance))
            {
                int next = (i+1) % 3;
                RandomlyPositionPipe(next);
                upperPipes[i].spawned = true;
                upperPipes[next].spawned = false;
                upperPipes[next].addedToScore = false;
            }
        }

        for (auto& pipe : upperPipes)
            if (pipe.UpdateScore())
                score++;

        // check for collisions
        for (auto& pipe : upperPipes)
            if (RectOverlap(bird.rect, pipe.rect))
                GameOver();

        for (auto& pipe : lowerPipes)
            if (RectOverlap(bird.rect, pipe.rect))
                GameOver();

        if (RectOverlap(bird.rect, ground))
            GameOver();
    }


    void RandomlyPositionPipe(int pipeIndex)
    {
        upperPipes[pipeIndex].rect.position.x = canvas_width;
        lowerPipes[pipeIndex].rect.position.x = canvas_width;

        int nextYOffset = random.GetInt(-maxOffset, maxOffset); // y distance from middle of screen where next pipe pair will be centered.
        int nextGapSize = random.GetInt(120, 160);
        lowerPipes[pipeIndex].rect.position.y = canvas_height/2 + nextYOffset + nextGapSize/2;
        upperPipes[pipeIndex].rect.position.y = canvas_height/2 + nextYOffset - nextGapSize/2 - pipeHeight;
    }

    void GameOver()
    {
        gameOver = true;
        bird.velocity.y = 0;

        for (auto& pipe : upperPipes)
            pipe.velocity.x = 0;

        for (auto& pipe : lowerPipes)
            pipe.velocity.x = 0;
    }

    void Draw()
    {
        canvas.Clear();

        //background
        canvas.Rect(0, 0, canvas_width, canvas_height, "#05BDED");

        for (auto& pipe : upperPipes)
            pipe.Draw(canvas);

        for (auto& pipe : lowerPipes)
            pipe.Draw(canvas);

        //ground
        canvas.Rect(ground.position.x, ground.position.y, ground.size.x, ground.size.y, "#1b2336");

        canvas.CenterText(canvas_width/2, 100, std::to_string(score), "white");

        bird.Draw(canvas);

        if (gameOver)
            canvas.CenterText(canvas_width/2, canvas_height/2, "Game Over!", "white");
    }

    void RouteInput ()
    {
        if (!gameOver)
        {
            bird.Flap();
            if (!startPipes)
                startPipes = true;
        }

        else
            StartGame();
    }


    void StartGame()
    {
        for (int i = 0; i < 3; i++)
        {
            upperPipes[i].rect.position.x = canvas_width + 120 + (i * pipeDistance);
            upperPipes[i].rect.size.x = 60;
            upperPipes[i].rect.size.y = pipeHeight;
            upperPipes[i].velocity.x = upperPipes[i].xSpeed;
            upperPipes[i].addedToScore = false;

            lowerPipes[i].rect.position.x = canvas_width + 120 + (i * pipeDistance);
            lowerPipes[i].rect.size.x = 60;
            lowerPipes[i].rect.size.y = pipeHeight;
            lowerPipes[i].velocity.x = lowerPipes[i].xSpeed;
            lowerPipes[i].addedToScore = false;

            bird.rect.position.x = canvas_width * 0.34;
            bird.rect.position.y = canvas_height * 0.5;
            bird.velocity.y = 0;

        }

        RandomlyPositionPipe(0);
        gameOver = false;
        startPipes = false;
        score = 0;

        Start();
    }


};


Game game;


void OnKeyPressed()
{
    game.RouteInput();
}





int main()
{
    keyManager.AddKeydownCallback(' ', &OnKeyPressed);
    game.StartGame();

    //doc << emp::web::Live(x);

}
