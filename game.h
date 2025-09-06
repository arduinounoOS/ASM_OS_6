const int rows = 5;
const int cols = 10;
int grid[rows][cols];  // Game grid
int playerCol = 5;     // Player position - bottom row
int score = 0;
unsigned long lastUpdate = 0;
const unsigned long updateInterval = 700;  // milliseconds
bool gameOn = 0;


// Initialize grid with zeros
void initializeGrid() {
  for (int r = 0; r < rows; r++) {
    for (int c = 0; c < cols; c++) {
      grid[r][c] = 0;
    }
  }
  grid[rows - 1][playerCol] = 2;  // Player position
}

// Move obstacles down one row
void moveObstaclesDown() {
  for (int r = rows - 2; r >= 0; r--) {
    for (int c = 0; c < cols; c++) {
      grid[r + 1][c] = grid[r][c];
    }
  }
  // Clear the top row
  for (int c = 0; c < cols; c++) {
    grid[0][c] = 0;
  }
  // Keep player at bottom
  for (int c = 0; c < cols; c++) {
    if (c == playerCol) grid[rows - 1][c] = 2;
    else grid[rows - 1][c] = 0;
  }
}

// Spawn a new obstacle at the top at random position
void spawnObstacle() {
  if (random(0, 5) == 0) {  // 1 in 3 chance
    int obstacleCol = random(0, cols);
    grid[0][obstacleCol] = 1;  // Obstacle
  }
}

// Check collision with obstacle
void checkCollision() {
  if (grid[rows - 1][playerCol] == 1) {
    score++;
    send(F("Ouch! Score: "));
    send(score);
    sendl();
    grid[rows - 1][playerCol] = 0;
  }
}

// Print the grid to serial
void printGrid() {
  sendl();
  sendl();
  sendl();
  sendl();
  sendl();

  for (int r = 0; r < rows; r++) {
    for (int c = 0; c < cols; c++) {
      if (grid[r][c] == 2) {
        send(F("X"));  // Player
      } else if (grid[r][c] == 1) {
        send(F("O"));  // Obstacle
      } else {
        send(F("."));
      }
    }
    sendl();
  }
  // Serial.print("Score: ");
  // Serial.println(score);
  // Serial.println("Move 'l' (left) or 'r' (right) and press Enter");
}
