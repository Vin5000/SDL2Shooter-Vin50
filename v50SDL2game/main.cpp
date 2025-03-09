//Using SDL, SDL_image, standard IO, vectors, and strings
#include <SDL.h>
#include <SDL_image.h>
#include <SDL_ttf.h>
#include <stdio.h>
#include <string>
#include <cmath>
#include <vector>
//The dimensions of the level
const int LEVEL_WIDTH = 1280;
const int LEVEL_HEIGHT = 960;
//Screen dimension constants
const int SCREEN_WIDTH = 850;
const int SCREEN_HEIGHT = 960;
//Texture wrapper class
class LTexture
{
public:
	//Initializes variables
	LTexture();

	//Deallocates memory
	~LTexture();

	//Loads image at specified path
	bool loadFromFile(std::string path);

	bool loadFromRenderedText(std::string textureText, SDL_Color textcolor);

	//Deallocates texture
	void free();

	//Set color modulation
	void setColor(Uint8 red, Uint8 green, Uint8 blue);

	//Set blending
	void setBlendMode(SDL_BlendMode blending);

	//Set alpha modulation
	void setAlpha(Uint8 alpha);

	//Renders texture at given point
	void render(int x, int y, SDL_Rect* clip = NULL, double angle = 0.0, SDL_Point* center = NULL, SDL_RendererFlip flip = SDL_FLIP_NONE);

	//Gets image dimensions
	int getWidth();
	int getHeight();

private:
	//The actual hardware texture
	SDL_Texture* mTexture;

	//Image dimensions
	int mWidth;
	int mHeight;
};

//The dot that will move around on the screen
class Dot
{
public:
	//The dimensions of the dot
	static const int DOT_WIDTH = 80;
	static const int DOT_HEIGHT = 80;

	//Maximum axis velocity of the dot
	static const int DOT_VEL = 10;

	//Initializes the variables
	Dot();

	//Takes key presses and adjusts the dot's velocity
	void handleEvent(SDL_Event& e);

	//Moves the dot
	void move(SDL_Rect enemyCollider);

	//Shows the dot on the screen relative to the camera
	void render(int camX, int camY);

	//Position accessors
	int getPosX();
	int getPosY();

	//movement
	enum DotState { IDLE, WALKING };

	DotState mState;

	SDL_Rect getCollider()
	{
		SDL_Rect collider = { mPosX, mPosY, DOT_WIDTH, DOT_HEIGHT };
		return collider;
	}



private:
	int mHealth;

	//The X and Y offsets of the dot
	int mPosX, mPosY;

	//The velocity of the dot
	int mVelX, mVelY;

	Uint32 lastDamageTime;

public:
	int getHealth() const { return mHealth; }  // Getter for health
	void reduceHealth(int amount) { mHealth -= amount; }
	void resetHealth() { mHealth = 100; }
};

class Enemy
{
public:
	static const int ENEMY_WIDTH = 20;
	static const int ENEMY_HEIGHT = 20;

	static const int ENEMY_VEL = 2;
	static const int ENEMY_MAX_HEALTH = 50;

	Enemy(int x, int y);

	void move();

	void render(int camX, int camY);


	void takeDamage(int amount) {
		health -= amount;
		if (health <= 0) {
			health = 0;
			// Handle enemy death logic (e.g., remove from vector)
		}
	}

	bool isDead() {
		return health <= 0;
	}

	SDL_Rect getCollider()
	{
		if (!isDead())
		{
			SDL_Rect collider = { mPosX, mPosY + 70, ENEMY_WIDTH, ENEMY_HEIGHT };
			return collider;
		}
		return{ 0,0,0,0 };
	}

	

	int health;


private:

	int mPosX, mPosY;

	int mVelX, mVelY;
};

//The window we'll be rendering to
SDL_Window* gWindow = NULL;

//The window renderer
SDL_Renderer* gRenderer = NULL;



class Projectile {
public:
	static const int PROJECTILE_WIDTH = 20;
	static const int PROJECTILE_HEIGHT = 10;
	static const int PROJECTILE_SPEED = 15;

	Projectile(int x, int y, int direction)
	{
		mPosX = x;
		mPosY = y;
		mDirection = direction;

		// Velocity is based on direction (left or right)
		mVelX = (mDirection == 1) ? PROJECTILE_SPEED : -PROJECTILE_SPEED;
	}

	void move()
	{
		mPosX += mVelX;
	}

	void render(int camX,int camY)
	{
		SDL_Rect fillRect = { mPosX, mPosY, PROJECTILE_WIDTH, PROJECTILE_HEIGHT };
		SDL_SetRenderDrawColor(gRenderer, 255, 0, 0, 255); // Red projectile
		SDL_RenderFillRect(gRenderer, &fillRect);

		SDL_Rect colRect = getCollider();
		colRect.x -= camX;
		colRect.y -= camY;
		SDL_SetRenderDrawColor(gRenderer, 255, 255, 255, 255);
		SDL_RenderDrawRect(gRenderer, &colRect);
	}

	bool isOffScreen()
	{
		return mPosX < 0 || mPosX > LEVEL_WIDTH;
	}

	SDL_Rect getCollider() {
		return { mPosX, mPosY, PROJECTILE_WIDTH, PROJECTILE_HEIGHT };
	}

private:
	int mPosX, mPosY;
	int mVelX;
	int mDirection;  // 1 = right, -1 = left
};

std::vector<Projectile> projectiles;

//Starts up SDL and creates window
bool init();

//Loads media
bool loadMedia();

//Frees media and shuts down SDL
void close();



TTF_Font* gFont = NULL;

LTexture gTextTexture;


//walking animation
const int WALKING_ANIMATION_FRAMES = 10;
SDL_Rect gSpriteClips[WALKING_ANIMATION_FRAMES];
LTexture gWalkingSheetTexture;

//IDLE animation
const int IDLE_ANIMATION_FRAMES = 6;
SDL_Rect gIdleClips[IDLE_ANIMATION_FRAMES];
LTexture gIdleSheetTexture;

//enemy animation
const int ENEMY_ANIMATION_FRAMES = 6;
SDL_Rect gEnemyclips[ENEMY_ANIMATION_FRAMES];
LTexture gEnemyTexture;
//Scene textures
LTexture gDotTexture;
LTexture gBGTexture;



SDL_RendererFlip flipType = SDL_FLIP_NONE;

LTexture::LTexture()
{
	//Initialize
	mTexture = NULL;
	mWidth = 0;
	mHeight = 0;
}

LTexture::~LTexture()
{
	//Deallocate
	free();
}

bool LTexture::loadFromFile(std::string path)
{
	//Get rid of preexisting texture
	free();

	//The final texture
	SDL_Texture* newTexture = NULL;

	//Load image at specified path
	SDL_Surface* loadedSurface = IMG_Load(path.c_str());
	if (loadedSurface == NULL)
	{
		printf("Unable to load image %s! SDL_image Error: %s\n", path.c_str(), IMG_GetError());
	}
	else
	{
		//Color key image

		SDL_SetColorKey(loadedSurface, SDL_TRUE, SDL_MapRGB(loadedSurface->format, 0, 0xFF, 0xFF));
		//Create texture from surface pixels
		newTexture = SDL_CreateTextureFromSurface(gRenderer, loadedSurface);
		if (newTexture == NULL)
		{
			printf("Unable to create texture from %s! SDL Error: %s\n", path.c_str(), SDL_GetError());
		}
		else
		{
			//Get image dimensions
			mWidth = loadedSurface->w;
			mHeight = loadedSurface->h;
		}

		//Get rid of old loaded surface
		SDL_FreeSurface(loadedSurface);
	}

	//Return success
	mTexture = newTexture;
	return mTexture != NULL;
}

bool LTexture::loadFromRenderedText(std::string textureText, SDL_Color textColor)
{
	free();
	SDL_Surface* textSurface = TTF_RenderText_Solid(gFont, textureText.c_str(), textColor);
	if (textSurface == NULL)
	{
		printf("unable to render text surface! SDL_ttf Error: %s\n", TTF_GetError());
	}
	else
	{
		mTexture = SDL_CreateTextureFromSurface(gRenderer, textSurface);
		if (mTexture == NULL)
		{
			printf("unable to create texture from rendered text! SDL Error: %s\n", SDL_GetError());

		}
		else
		{
			mWidth = textSurface->w;
			mHeight = textSurface->h;
		}
		SDL_FreeSurface(textSurface);
	}
	return mTexture != NULL;
}

bool checkCollision(SDL_Rect a, SDL_Rect b)
{
	// The sides of the rectangles
	int leftA = a.x;
	int rightA = a.x + a.w;
	int topA = a.y;
	int bottomA = a.y + a.h;

	int leftB = b.x;
	int rightB = b.x + b.w;
	int topB = b.y;
	int bottomB = b.y + b.h;

	// Check if rectangles overlap
	if (bottomA <= topB)
	{
		return false;
	}

	if (topA >= bottomB)
	{
		return false;
	}

	if (rightA <= leftB)
	{
		return false;
	}

	if (leftA >= rightB)
	{
		return false;
	}

	// If none of the sides are outside, the rectangles overlap
	return true;
}



void LTexture::free()
{
	//Free texture if it exists
	if (mTexture != NULL)
	{
		SDL_DestroyTexture(mTexture);
		mTexture = NULL;
		mWidth = 0;
		mHeight = 0;
	}
}

void LTexture::setColor(Uint8 red, Uint8 green, Uint8 blue)
{
	//Modulate texture rgb
	SDL_SetTextureColorMod(mTexture, red, green, blue);
}

void LTexture::setBlendMode(SDL_BlendMode blending)
{
	//Set blending function
	SDL_SetTextureBlendMode(mTexture, blending);
}

void LTexture::setAlpha(Uint8 alpha)
{
	//Modulate texture alpha
	SDL_SetTextureAlphaMod(mTexture, alpha);
}

void LTexture::render(int x, int y, SDL_Rect* clip, double angle, SDL_Point* center, SDL_RendererFlip flip)
{
	//Set rendering space and render to screen
	SDL_Rect renderQuad = { x, y, mWidth, mHeight };

	//Set clip rendering dimensions
	if (clip != NULL)
	{
		renderQuad.w = clip->w;
		renderQuad.h = clip->h;
	}

	//Render to screen
	SDL_RenderCopyEx(gRenderer, mTexture, clip, &renderQuad, angle, center, flip);
}

int LTexture::getWidth()
{
	return mWidth;
}

int LTexture::getHeight()
{
	return mHeight;
}

Dot::Dot()
{
	//Initialize the offsets
	mPosX = 30;
	mPosY = 800;

	//Initialize the velocity
	mVelX = 0;
	mVelY = 0;
	mState = IDLE;

	mHealth = 100;
	lastDamageTime = 0;
}
Enemy::Enemy(int x, int y)
{
	mPosX = x;
	mPosY = y;

	mVelX = 0;
	mVelY = 0;
	health = ENEMY_MAX_HEALTH;
}

void Dot::handleEvent(SDL_Event& e)
{
	if (e.type == SDL_KEYDOWN && e.key.repeat == 0)
	{
		switch (e.key.keysym.sym)
		{
		case SDLK_LEFT:
			mVelX -= DOT_VEL;
			flipType = SDL_FLIP_HORIZONTAL;
			mState = WALKING;
			break;
		case SDLK_RIGHT:
			mVelX += DOT_VEL;
			flipType = SDL_FLIP_NONE;
			mState = WALKING;
			break;
		case SDLK_SPACE: // Fire projectile
		{
			int direction = (flipType == SDL_FLIP_NONE) ? 1 : -1;
			int projX = (direction == 1) ? mPosX + DOT_WIDTH : mPosX - 20;
			int projY = mPosY + 85;
			projectiles.emplace_back(projX, projY, direction);
			break;
		}
		}
	}
	else if (e.type == SDL_KEYUP && e.key.repeat == 0)
	{
		switch (e.key.keysym.sym)
		{
		case SDLK_LEFT:
			mVelX += DOT_VEL;
			break;
		case SDLK_RIGHT:
			mVelX -= DOT_VEL;
			break;
		}
		if (mVelX == 0)
		{
			mState = IDLE;
		}
	}
}

void Dot::move(SDL_Rect enemyCollider)
{
	mPosX += mVelX;

	if ((mPosX < 0) || (mPosX + DOT_WIDTH > LEVEL_WIDTH))
	{
		mPosX -= mVelX;
	}

	if (checkCollision(getCollider(), enemyCollider))
	{
		mPosX -= mVelX;
		Uint32 currentTime = SDL_GetTicks();
		if (currentTime - lastDamageTime >= 3000)  // 3000 ms = 3 seconds
		{
			reduceHealth(25);
			lastDamageTime = currentTime;  // Reset timer
		}
	}

	mPosY += mVelY;

	if ((mPosY < 0) || (mPosY + DOT_HEIGHT > LEVEL_HEIGHT))
	{
		mPosY -= mVelY;
	}

	if (checkCollision(getCollider(), enemyCollider))
	{
		mPosY -= mVelY;
		Uint32 currentTime = SDL_GetTicks();
		if (currentTime - lastDamageTime >= 3000)
		{
			reduceHealth(25);
			lastDamageTime = currentTime;
		}
	}
}

void Enemy::move()
{
	mPosX += mVelX;

	if ((mPosX < 0) || (mPosX + ENEMY_WIDTH > SCREEN_WIDTH))
	{
		mPosX -= mVelX;
	}

	mPosY += mVelY;

	if ((mPosY < 0) || (mPosY + ENEMY_HEIGHT > SCREEN_HEIGHT))
	{
		mPosY -= mVelY;
	}
	
}

void Dot::render(int camX, int camY)
{
	SDL_Rect* currentClip = nullptr;

	if (mState == WALKING)
	{
		currentClip = &gSpriteClips[SDL_GetTicks() / 100 % WALKING_ANIMATION_FRAMES];  // Cycle through walking animation
		gWalkingSheetTexture.render(mPosX - camX, mPosY - camY, currentClip, 0.0, NULL, flipType);
	}
	else
	{
		currentClip = &gIdleClips[SDL_GetTicks() / 100 % IDLE_ANIMATION_FRAMES];  // Cycle through idle animation
		gIdleSheetTexture.render(mPosX - camX, mPosY - camY, currentClip, 0.0, NULL, flipType);
	}
	SDL_Rect colRect = getCollider();
	colRect.x -= camX;
	colRect.y -= camY;
	SDL_SetRenderDrawColor(gRenderer, 255, 255, 255, 255);
	SDL_RenderDrawRect(gRenderer, &colRect);
}

void Enemy::render(int camX, int camY)
{
	SDL_Rect* currentClip = &gEnemyclips[SDL_GetTicks() / 100 % ENEMY_ANIMATION_FRAMES];
	gEnemyTexture.render(mPosX - camX, mPosY - camY, currentClip);

	SDL_Color textColor = { 255, 0, 0 };  // Red color for health
	LTexture healthTexture;

	std::string healthText = std::to_string(health);  // Convert health to string
	if (healthTexture.loadFromRenderedText(healthText, textColor))
	{
		healthTexture.render(mPosX - camX, mPosY - camY - 20); // Position above enemy
	}
	SDL_Rect colRect = getCollider();
	colRect.x -= camX;
	colRect.y -= camY;
	SDL_SetRenderDrawColor(gRenderer, 255, 255, 255, 255);
	SDL_RenderDrawRect(gRenderer, &colRect);
}

int Dot::getPosX()
{
	return mPosX;
}

int Dot::getPosY()
{
	return mPosY;
}

bool init()
{
	//Initialization flag
	bool success = true;

	//Initialize SDL
	if (SDL_Init(SDL_INIT_VIDEO) < 0)
	{
		printf("SDL could not initialize! SDL Error: %s\n", SDL_GetError());
		success = false;
	}
	else
	{
		//Set texture filtering to linear
		if (!SDL_SetHint(SDL_HINT_RENDER_SCALE_QUALITY, "1"))
		{
			printf("Warning: Linear texture filtering not enabled!");
		}

		//Create window
		gWindow = SDL_CreateWindow("Mafia Street Brawl", SDL_WINDOWPOS_UNDEFINED, SDL_WINDOWPOS_UNDEFINED, SCREEN_WIDTH, SCREEN_HEIGHT, SDL_WINDOW_SHOWN);
		if (gWindow == NULL)
		{
			printf("Window could not be created! SDL Error: %s\n", SDL_GetError());
			success = false;
		}
		else
		{
			//Create vsynced renderer for window
			gRenderer = SDL_CreateRenderer(gWindow, -1, SDL_RENDERER_ACCELERATED | SDL_RENDERER_PRESENTVSYNC);
			if (gRenderer == NULL)
			{
				printf("Renderer could not be created! SDL Error: %s\n", SDL_GetError());
				success = false;
			}
			else
			{
				//Initialize renderer color
				SDL_SetRenderDrawColor(gRenderer, 0xFF, 0xFF, 0xFF, 0xFF);

				//Initialize PNG loading
				int imgFlags = IMG_INIT_PNG;
				if (!(IMG_Init(imgFlags) & imgFlags))
				{
					printf("SDL_image could not initialize! SDL_image Error: %s\n", IMG_GetError());
					success = false;
				}

				if (TTF_Init() == -1)
				{
					printf("SDL_ttf could not initialize! SDL_ttf Error: %s\n", TTF_GetError());
					success = false;
				}
			}
		}
	}

	return success;
}

bool loadMedia()
{
	//Loading success flag
	bool success = true;

	//Load dot texture
	if (!gIdleSheetTexture.loadFromFile("Gangsters_1/Idle.png"))
	{
		printf("Failed to load dot texture!\n");
		success = false;
	}
	else
	{
		gIdleClips[0].x = 44;
		gIdleClips[0].y = 0;
		gIdleClips[0].w = 100;
		gIdleClips[0].h = 155;

		gIdleClips[1].x = 171;
		gIdleClips[1].y = 0;
		gIdleClips[1].w = 100;
		gIdleClips[1].h = 155;

		gIdleClips[2].x = 298;
		gIdleClips[2].y = 0;
		gIdleClips[2].w = 100;
		gIdleClips[2].h = 155;

		gIdleClips[3].x = 425;
		gIdleClips[3].y = 0;
		gIdleClips[3].w = 100;
		gIdleClips[3].h = 155;

		gIdleClips[4].x = 552;
		gIdleClips[4].y = 0;
		gIdleClips[4].w = 100;
		gIdleClips[4].h = 155;

		gIdleClips[5].x = 679;
		gIdleClips[5].y = 0;
		gIdleClips[5].w = 100;
		gIdleClips[5].h = 155;


	}

	if (!gWalkingSheetTexture.loadFromFile("Gangsters_1/Run.png"))
	{
		printf("Failed to load walking texture");
		success = false;
	}
	else
	{
		gSpriteClips[0].x = 38;
		gSpriteClips[0].y = 0;
		gSpriteClips[0].w = 100;
		gSpriteClips[0].h = 155;

		gSpriteClips[1].x = 168;
		gSpriteClips[1].y = 0;
		gSpriteClips[1].w = 100;
		gSpriteClips[1].h = 155;

		gSpriteClips[2].x = 290;
		gSpriteClips[2].y = 0;
		gSpriteClips[2].w = 100;
		gSpriteClips[2].h = 155;

		gSpriteClips[3].x = 410;
		gSpriteClips[3].y = 0;
		gSpriteClips[3].w = 100;
		gSpriteClips[3].h = 155;

		gSpriteClips[4].x = 545;
		gSpriteClips[4].y = 0;
		gSpriteClips[4].w = 100;
		gSpriteClips[4].h = 155;

		gSpriteClips[5].x = 665;
		gSpriteClips[5].y = 0;
		gSpriteClips[5].w = 100;
		gSpriteClips[5].h = 155;
	}


	//Load background texture
	if (!gBGTexture.loadFromFile("City3/Bright/City3.png"))
	{
		printf("Failed to load background texture!\n");
		success = false;
	}
	//font
	gFont = TTF_OpenFont("lazy.ttf", 28);
	if (gFont == NULL)
	{
		printf("failed to load lazy font! SDL_ttf Error: %s\n", TTF_GetError());
		success = false;
	}
	else
	{
		SDL_Color textColor = { 0,255,0 };
		if (!gTextTexture.loadFromRenderedText("Health", textColor))
		{
			printf("Failed to render text texture!\n");
			success = false;
		}
	}

	if (!gEnemyTexture.loadFromFile("Gangsters_2/Idle.png"))
	{
		printf("Failed to load enemy texture!\n");
		success = false;
	}
	else
	{

		gEnemyclips[0].x = 38;
		gEnemyclips[0].y = 0;
		gEnemyclips[0].w = 100;
		gEnemyclips[0].h = 155;

		gEnemyclips[1].x = 168;
		gEnemyclips[1].y = 0;
		gEnemyclips[1].w = 100;
		gEnemyclips[1].h = 155;

		gEnemyclips[2].x = 290;
		gEnemyclips[2].y = 0;
		gEnemyclips[2].w = 100;
		gEnemyclips[2].h = 155;

		gEnemyclips[3].x = 410;
		gEnemyclips[3].y = 0;
		gEnemyclips[3].w = 100;
		gEnemyclips[3].h = 155;

		gEnemyclips[4].x = 545;
		gEnemyclips[4].y = 0;
		gEnemyclips[4].w = 100;
		gEnemyclips[4].h = 155;

		gEnemyclips[5].x = 665;
		gEnemyclips[5].y = 0;
		gEnemyclips[5].w = 100;
		gEnemyclips[5].h = 155;
	}
	return success;
}

void close()
{
	//Free loaded images
	gDotTexture.free();
	gBGTexture.free();

	TTF_CloseFont(gFont);
	gFont = NULL;


	//Destroy window	
	SDL_DestroyRenderer(gRenderer);
	SDL_DestroyWindow(gWindow);
	gWindow = NULL;
	gRenderer = NULL;


	//Quit SDL subsystems
	TTF_Quit();
	IMG_Quit();
	SDL_Quit();
}
//check if this code is being used

int main(int argc, char* args[])
{
	//Start up SDL and create window
	if (!init())
	{
		printf("Failed to initialize!\n");
	}
	else
	{
		//Load media
		if (!loadMedia())
		{
			printf("Failed to load media!\n");
		}
		else
		{
			//Main loop flag
			bool quit = false;

			//Event handler
			SDL_Event e;

			//Current animation frame
			int frame = 0;
			//The dot that will be moving around on the screen
			Dot dot;

			Enemy Enemy(900, 800);

			//The camera area
			SDL_Rect camera = { 50, 50, SCREEN_WIDTH, SCREEN_HEIGHT };

			//While application is running
			while (!quit)
			{
				//Handle events on queue
				while (SDL_PollEvent(&e) != 0)
				{
					//User requests quit
					if (e.type == SDL_QUIT)
					{
						quit = true;
					}

					//Handle input for the dot
					dot.handleEvent(e);

				}

				//Move the dot

			
				
					dot.move(Enemy.getCollider());
				
				
					Enemy.move();
				

				//Center the camera over the dot
				camera.x = (dot.getPosX() + Dot::DOT_WIDTH / 2) - SCREEN_WIDTH / 2;
				camera.y = (dot.getPosY() + Dot::DOT_HEIGHT / 2) - SCREEN_HEIGHT / 2;

				//Keep the camera in bounds
				if (camera.x < 0)
				{
					camera.x = 0;
				}
				if (camera.y < 0)
				{
					camera.y = 0;
				}
				if (camera.x > LEVEL_WIDTH - camera.w)
				{
					camera.x = LEVEL_WIDTH - camera.w;
				}
				if (camera.y > LEVEL_HEIGHT - camera.h)
				{
					camera.y = LEVEL_HEIGHT - camera.h;
				}

				//Clear screen
				SDL_SetRenderDrawColor(gRenderer, 0xFF, 0xFF, 0xFF, 0xFF);
				SDL_RenderClear(gRenderer);

				//Render background
				gBGTexture.render(0, 0, &camera);

				//Render objects
				dot.render(camera.x, camera.y);

				gTextTexture.render(0, 0);

				Enemy.render(camera.x, camera.y);

				for (size_t i = 0; i < projectiles.size();)
				{
					projectiles[i].move();
					projectiles[i].render(camera.x,camera.y);

					if (projectiles[i].isOffScreen())
					{
						projectiles.erase(projectiles.begin() + i);
					}
					else
					{
						++i;
					}
				}

				// Update projectiles
				for (auto it = projectiles.begin(); it != projectiles.end(); ) {
					it->move();

					// If the projectile collides with the enemy
					if (Enemy.isDead())
					{
						gEnemyTexture.free();
						
						
					}
					if (!Enemy.isDead() && checkCollision(it->getCollider(), Enemy.getCollider())) {
						Enemy.takeDamage(10);
						it = projectiles.erase(it); // Remove projectile after hit
					}
					else if (it->isOffScreen()) {
						it = projectiles.erase(it); // Remove projectile if off-screen
					}
					else {
						++it;
					}
				}
				for (auto& proj : projectiles)
				{
					proj.render(camera.x, camera.y);
				}

				//Update screen
				SDL_RenderPresent(gRenderer);

				//health
				SDL_Color textColor = { 255, 255, 255 };  // White color
				std::string healthText = "Health: " + std::to_string(dot.getHealth());

				//SDL_Color EtextColor = { 255, 255, 255 };
				//std::string EhealthText = "Enemy health: " + std::to_string(Enemy.health());

				gTextTexture.loadFromRenderedText(healthText, textColor);
				gTextTexture.render(10, 10);  // Render at top left corner


				//Go to next frame
				++frame;

				//Cycle animation
				if (frame / 6 >= (dot.mState == Dot::WALKING ? WALKING_ANIMATION_FRAMES : IDLE_ANIMATION_FRAMES)) {
					frame = 0;
				}
			}
		}
	}
	//Free resources and close SDL
	close();

	return 0;
}
