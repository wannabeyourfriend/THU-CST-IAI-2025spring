#include <iostream>
#include <unistd.h>
#include <cstring>
#include <cstdlib>
#include <ctime>
#include <vector>
#include <algorithm>
#include <cmath>
#include <chrono>
#include "Point.h"
#include "Strategy.h"
#include "Judge.h"

using namespace std;
using namespace chrono;

// 定义常量
const double UCT_CONSTANT = 1.414; // UCB1公式中的常数
const int MAX_SIMULATION = 10000; // 最大模拟次数
const int MAX_DEPTH = 10; // 最大搜索深度
const double TIME_LIMIT = 2.8; // 时间限制(秒)

// 函数声明
void clearArray(int M, int N, int **board);
int countPieces(const int M, const int N, int **board);

// 游戏状态类
class GameState {
public:
    int M, N; // 棋盘大小
    int **board; // 棋盘
    int *top; // 每列的顶部位置
    int lastX, lastY; // 上一步落子位置
    int currentPlayer; // 当前玩家 (1-用户, 2-AI)

    // 构造函数
    GameState(const int M, const int N, int **board, const int *top, int lastX, int lastY, int currentPlayer) {
        this->M = M;
        this->N = N;
        this->lastX = lastX;
        this->lastY = lastY;
        this->currentPlayer = currentPlayer;

        // 复制棋盘
        this->board = new int*[M];
        for (int i = 0; i < M; i++) {
            this->board[i] = new int[N];
            for (int j = 0; j < N; j++) {
                this->board[i][j] = board[i][j];
            }
        }

        // 复制top数组
        this->top = new int[N];
        for (int i = 0; i < N; i++) {
            this->top[i] = top[i];
        }
    }

    // 拷贝构造函数
    GameState(const GameState &state) {
        M = state.M;
        N = state.N;
        lastX = state.lastX;
        lastY = state.lastY;
        currentPlayer = state.currentPlayer;

        // 复制棋盘
        board = new int*[M];
        for (int i = 0; i < M; i++) {
            board[i] = new int[N];
            for (int j = 0; j < N; j++) {
                board[i][j] = state.board[i][j];
            }
        }

        // 复制top数组
        top = new int[N];
        for (int i = 0; i < N; i++) {
            top[i] = state.top[i];
        }
    }

    // 析构函数
    ~GameState() {
        for (int i = 0; i < M; i++) {
            delete[] board[i];
        }
        delete[] board;
        delete[] top;
    }

    // 获取可用的落子点
    vector<int> getAvailableMoves() {
        vector<int> moves;
        for (int i = 0; i < N; i++) {
            if (top[i] > 0) {
                moves.push_back(i);
            }
        }
        return moves;
    }

    // 执行一步落子
    bool makeMove(int col) {
        if (col < 0 || col >= N || top[col] <= 0) {
            return false; // 无效的落子
        }

        int row = top[col] - 1;
        board[row][col] = currentPlayer;
        top[col]--;
        lastX = row;
        lastY = col;

        // 切换玩家
        currentPlayer = 3 - currentPlayer; // 1->2, 2->1
        return true;
    }

    // 检查游戏是否结束
    bool isGameOver(int &winner) {
        // 检查上一步落子是否导致胜利
        if (lastX >= 0 && lastY >= 0) {
            if (currentPlayer == 1) { // 上一步是AI落子
                if (machineWin(lastX, lastY, M, N, board)) {
                    winner = 2; // AI赢
                    return true;
                }
            } else { // 上一步是用户落子
                if (userWin(lastX, lastY, M, N, board)) {
                    winner = 1; // 用户赢
                    return true;
                }
            }
        }

        // 检查平局
        if (isTie(N, top)) {
            winner = 0; // 平局
            return true;
        }

        winner = -1; // 游戏未结束
        return false;
    }

    // 评估函数 - 评估当前棋盘状态对AI的有利程度
    int evaluate() {
        int score = 0;
        int winner = -1;
        
        // 如果游戏已结束，直接返回结果
        if (isGameOver(winner)) {
            if (winner == 2) return 10000; // AI赢
            if (winner == 1) return -10000; // 用户赢
            return 0; // 平局
        }
        
        // 评估连续的棋子
        score += evaluateLines();
        
        // 评估中心控制
        score += evaluateCenter();
        
        // 评估威胁和机会
        score += evaluateThreats();
        
        return score;
    }
    
    // 评估中心控制
    int evaluateCenter() {
        int score = 0;
        int centerCol = N / 2;
        
        // 中心列权重
        for (int i = 0; i < M; i++) {
            if (board[i][centerCol] == 2) { // AI棋子
                score += 30;
            } else if (board[i][centerCol] == 1) { // 用户棋子
                score -= 30;
            }
        }
        
        // 中心区域权重 (中心列及其相邻列)
        for (int j = max(0, centerCol - 1); j <= min(N - 1, centerCol + 1); j++) {
            if (j == centerCol) continue; // 中心列已经计算过
            
            for (int i = 0; i < M; i++) {
                if (board[i][j] == 2) { // AI棋子
                    score += 15;
                } else if (board[i][j] == 1) { // 用户棋子
                    score -= 15;
                }
            }
        }
        
        return score;
    }
    
    // 评估威胁和机会
    int evaluateThreats() {
        int score = 0;
        
        // 检查是否有立即获胜的机会
        for (int j = 0; j < N; j++) {
            if (top[j] <= 0) continue; // 列已满
            
            int row = top[j] - 1;
            
            // 模拟AI在此处落子
            board[row][j] = 2;
            if (machineWin(row, j, M, N, board)) {
                score += 5000; // 立即获胜的机会
            }
            
            // 模拟用户在此处落子
            board[row][j] = 1;
            if (userWin(row, j, M, N, board)) {
                score -= 5000; // 必须阻止的威胁
            }
            
            // 恢复棋盘
            board[row][j] = 0;
        }
        
        return score;
    }
    
    // 评估一条线上的连续棋子 (增强版)
    int evaluateLine(int startX, int startY, int dirX, int dirY, int length) {
        int aiCount = 0;
        int userCount = 0;
        int emptyCount = 0;
        int emptyPositions[4] = {-1, -1, -1, -1}; // 记录空位置的索引
        int emptyIndex = 0;
        
        for (int i = 0; i < length; i++) {
            int x = startX + i * dirX;
            int y = startY + i * dirY;
            
            if (board[x][y] == 2) aiCount++;
            else if (board[x][y] == 1) userCount++;
            else {
                emptyCount++;
                emptyPositions[emptyIndex++] = i;
            }
        }
        
        // 如果同时有AI和用户的棋子，这条线没有威胁
        if (aiCount > 0 && userCount > 0) return 0;
        
        // 评分 (增强版)
        if (aiCount == 3 && emptyCount == 1) {
            // 检查空位是否可以落子 (是否在底部或有支撑)
            int emptyPos = emptyPositions[0];
            int x = startX + emptyPos * dirX;
            int y = startY + emptyPos * dirY;
            
            // 如果是底行或下方有棋子，则是真正的威胁
            if (x == M - 1 || (x + 1 < M && board[x + 1][y] != 0)) {
                return 2000; // 更高的权重，因为这是真正的威胁
            }
            return 500; // 普通的三子连线
        }
        
        if (aiCount == 2 && emptyCount == 2) {
            // 检查是否形成了潜在的双威胁
            bool potentialThreat = false;
            for (int i = 0; i < emptyCount; i++) {
                int emptyPos = emptyPositions[i];
                int x = startX + emptyPos * dirX;
                int y = startY + emptyPos * dirY;
                
                // 如果是底行或下方有棋子，则可以在此处落子
                if (x == M - 1 || (x + 1 < M && board[x + 1][y] != 0)) {
                    potentialThreat = true;
                    break;
                }
            }
            
            return potentialThreat ? 200 : 50;
        }
        
        if (aiCount == 1 && emptyCount == 3) return 10;
        
        // 用户威胁评估，类似地进行增强
        if (userCount == 3 && emptyCount == 1) {
            int emptyPos = emptyPositions[0];
            int x = startX + emptyPos * dirX;
            int y = startY + emptyPos * dirY;
            
            if (x == M - 1 || (x + 1 < M && board[x + 1][y] != 0)) {
                return -2000; // 更高的权重，这是真正的威胁
            }
            return -500;
        }
        
        if (userCount == 2 && emptyCount == 2) {
            bool potentialThreat = false;
            for (int i = 0; i < emptyCount; i++) {
                int emptyPos = emptyPositions[i];
                int x = startX + emptyPos * dirX;
                int y = startY + emptyPos * dirY;
                
                if (x == M - 1 || (x + 1 < M && board[x + 1][y] != 0)) {
                    potentialThreat = true;
                    break;
                }
            }
            
            return potentialThreat ? -200 : -50;
        }
        
        if (userCount == 1 && emptyCount == 3) return -10;
        
        return 0;
    }
    
    // 评估棋盘上的连续棋子
    int evaluateLines() {
        int score = 0;
        
        // 水平方向
        for (int i = 0; i < M; i++) {
            for (int j = 0; j <= N - 4; j++) {
                score += evaluateLine(i, j, 0, 1, 4);
            }
        }
        
        // 垂直方向
        for (int i = 0; i <= M - 4; i++) {
            for (int j = 0; j < N; j++) {
                score += evaluateLine(i, j, 1, 0, 4);
            }
        }
        
        // 对角线方向 (左上到右下)
        for (int i = 0; i <= M - 4; i++) {
            for (int j = 0; j <= N - 4; j++) {
                score += evaluateLine(i, j, 1, 1, 4);
            }
        }
        
        // 对角线方向 (右上到左下)
        for (int i = 0; i <= M - 4; i++) {
            for (int j = 3; j < N; j++) {
                score += evaluateLine(i, j, 1, -1, 4);
            }
        }
        
        return score;
    }
};

// MCTS节点类
class MCTSNode {
public:
    GameState *state; // 游戏状态
    MCTSNode *parent; // 父节点
    vector<MCTSNode*> children; // 子节点
    int visits; // 访问次数
    double wins; // 胜利次数
    vector<int> untriedMoves; // 未尝试的落子点

    // 构造函数
    MCTSNode(GameState *state, MCTSNode *parent = nullptr) {
        this->state = state;
        this->parent = parent;
        visits = 0;
        wins = 0.0;
        untriedMoves = state->getAvailableMoves();
    }

    // 析构函数
    ~MCTSNode() {
        for (MCTSNode *child : children) {
            delete child;
        }
        delete state;
    }

    // 选择最佳子节点 (UCB1)
    MCTSNode* selectBestChild() {
        double bestScore = -1e9;
        MCTSNode *bestChild = nullptr;

        for (MCTSNode *child : children) {
            double exploitation = child->wins / child->visits;
            double exploration = UCT_CONSTANT * sqrt(2 * log(visits) / child->visits);
            double score = exploitation + exploration;

            if (score > bestScore) {
                bestScore = score;
                bestChild = child;
            }
        }

        return bestChild;
    }

    // 扩展节点
    MCTSNode* expand() {
        if (untriedMoves.empty()) {
            return nullptr;
        }

        // 随机选择一个未尝试的落子点
        int index = rand() % untriedMoves.size();
        int move = untriedMoves[index];
        untriedMoves.erase(untriedMoves.begin() + index);

        // 创建新的游戏状态
        GameState *newState = new GameState(*state);
        newState->makeMove(move);

        // 创建新的子节点
        MCTSNode *child = new MCTSNode(newState, this);
        children.push_back(child);

        return child;
    }

    // 模拟随机游戏
    int simulate() {
        GameState simulationState(*state);
        int winner = -1;

        // 随机模拟直到游戏结束
        while (!simulationState.isGameOver(winner)) {
            vector<int> moves = simulationState.getAvailableMoves();
            if (moves.empty()) break;

            // 随机选择一个落子点
            int move = moves[rand() % moves.size()];
            simulationState.makeMove(move);
        }

        return winner;
    }

    // 回溯更新节点统计信息
    void backpropagate(int result) {
        visits++;
        if (result == 2) { // AI赢
            wins += 1.0;
        } else if (result == 0) { // 平局
            wins += 0.5;
        }

        if (parent != nullptr) {
            parent->backpropagate(result);
        }
    }

    // 获取最佳落子点
    int getBestMove() {
        int bestMove = -1;
        double bestWinRate = -1;

        for (MCTSNode *child : children) {
            double winRate = child->wins / child->visits;
            if (winRate > bestWinRate) {
                bestWinRate = winRate;
                bestMove = child->state->lastY;
            }
        }

        return bestMove;
    }
};

// 使用启发式的模拟函数声明
int simulateWithHeuristics(GameState state);

// 优化的蒙特卡洛树搜索
int monteCarloTreeSearch(const int M, const int N, const int *top, int **board, const int lastX, const int lastY) {
    // 创建根节点
    GameState *rootState = new GameState(M, N, board, top, lastX, lastY, 2); // AI是玩家2
    MCTSNode *root = new MCTSNode(rootState);

    // 记录开始时间
    auto startTime = high_resolution_clock::now();
    int simulations = 0;

    // 运行MCTS算法
    while (simulations < MAX_SIMULATION) {
        // 检查时间限制
        auto currentTime = high_resolution_clock::now();
        double elapsedTime = duration_cast<duration<double>>(currentTime - startTime).count();
        if (elapsedTime >= TIME_LIMIT) break;

        // 1. 选择
        MCTSNode *node = root;
        while (node->untriedMoves.empty() && !node->children.empty()) {
            node = node->selectBestChild();
        }

        // 2. 扩展
        if (!node->untriedMoves.empty()) {
            // 优先考虑中心列
            int middleCol = N / 2;
            bool expandedMiddle = false;
            
            // 检查未尝试的移动中是否有中心列
            for (size_t i = 0; i < node->untriedMoves.size(); i++) {
                if (node->untriedMoves[i] == middleCol) {
                    // 优先扩展中心列
                    int move = node->untriedMoves[i];
                    node->untriedMoves.erase(node->untriedMoves.begin() + i);
                    
                    // 创建新的游戏状态
                    GameState *newState = new GameState(*node->state);
                    newState->makeMove(move);
                    
                    // 创建新的子节点
                    MCTSNode *child = new MCTSNode(newState, node);
                    node->children.push_back(child);
                    
                    node = child;
                    expandedMiddle = true;
                    break;
                }
            }
            
            // 如果没有扩展中心列，则随机扩展
            if (!expandedMiddle) {
                node = node->expand();
            }
        }

        // 3. 模拟 - 使用启发式模拟而不是完全随机
        int result = simulateWithHeuristics(*node->state);

        // 4. 回溯
        node->backpropagate(result);

        simulations++;
    }

    // 选择最佳落子点
    int bestMove = root->getBestMove();

    // 清理内存
    delete root;

    return bestMove;
}

// 使用启发式的模拟
int simulateWithHeuristics(GameState state) {
    int winner = -1;
    int moveCount = 0;
    const int MAX_MOVES = 30; // 防止无限循环

    // 随机模拟直到游戏结束或达到最大移动次数
    while (!state.isGameOver(winner) && moveCount < MAX_MOVES) {
        vector<int> moves = state.getAvailableMoves();
        if (moves.empty()) break;

        // 检查是否有立即获胜的移动
        int winningMove = -1;
        int blockingMove = -1;
        
        for (int move : moves) {
            // 检查这个移动是否会导致当前玩家获胜
            GameState testState(state);
            testState.makeMove(move);
            int testWinner = -1;
            if (testState.isGameOver(testWinner) && testWinner == 3 - state.currentPlayer) {
                winningMove = move;
                break;
            }
            
            // 检查这个移动是否会阻止对手获胜
            // 模拟对手在这里落子
            GameState blockTest(state);
            blockTest.currentPlayer = 3 - blockTest.currentPlayer; // 切换到对手
            blockTest.makeMove(move);
            int blockTestWinner = -1;
            if (blockTest.isGameOver(blockTestWinner) && blockTestWinner == blockTest.currentPlayer) {
                blockingMove = move;
            }
        }
        
        int selectedMove;
        
        if (winningMove != -1) {
            // 如果有获胜移动，选择它
            selectedMove = winningMove;
        } else if (blockingMove != -1) {
            // 如果有阻止对手获胜的移动，选择它
            selectedMove = blockingMove;
        } else {
            // 否则，偏好中心列及其附近的列
            int middleCol = state.N / 2;
            vector<pair<int, int>> moveDistances;
            
            for (int move : moves) {
                moveDistances.push_back({move, abs(move - middleCol)});
            }
            
            // 按照到中心列的距离排序
            sort(moveDistances.begin(), moveDistances.end(), 
                 [](const pair<int, int> &a, const pair<int, int> &b) {
                     return a.second < b.second; // 升序，优先选择接近中心的列
                 });
            
            // 有80%的概率选择最接近中心的列，20%的概率随机选择
            if (rand() % 100 < 80 && !moveDistances.empty()) {
                selectedMove = moveDistances[0].first;
            } else {
                selectedMove = moves[rand() % moves.size()];
            }
        }
        
        state.makeMove(selectedMove);
        moveCount++;
    }

    // 如果达到最大移动次数但游戏未结束，使用评估函数
    if (moveCount >= MAX_MOVES && winner == -1) {
        int score = state.evaluate();
        if (score > 0) return 2; // AI有优势
        if (score < 0) return 1; // 用户有优势
        return 0; // 平局
    }

    return winner;
}
// 优化的Alpha-Beta剪枝的极小极大搜索
int minimax(GameState &state, int depth, int alpha, int beta, bool maximizingPlayer, bool isRoot = false) {
    // 立即检查终局状态
    int winner = -1;
    if (state.isGameOver(winner)) {
        if (winner == 2) return 10000 + depth; // AI赢，尽快结束游戏
        if (winner == 1) return -10000 - depth; // 用户赢，尽量拖延
        return 0; // 平局
    }
    
    // 达到最大深度
    if (depth == 0) {
        return state.evaluate();
    }

    vector<int> moves = state.getAvailableMoves();
    
    // 对移动进行排序以提高剪枝效率
    if (isRoot || moves.size() > 3) {
        vector<pair<int, int>> moveScores;
        for (int move : moves) {
            GameState newState(state);
            newState.makeMove(move);
            moveScores.push_back({move, newState.evaluate()});
        }
        
        // 根据评分排序移动
        if (maximizingPlayer) {
            sort(moveScores.begin(), moveScores.end(), 
                 [](const pair<int, int> &a, const pair<int, int> &b) {
                     return a.second > b.second; // 降序
                 });
        } else {
            sort(moveScores.begin(), moveScores.end(), 
                 [](const pair<int, int> &a, const pair<int, int> &b) {
                     return a.second < b.second; // 升序
                 });
        }
        
        moves.clear();
        for (const auto &p : moveScores) {
            moves.push_back(p.first);
        }
    }
    
    // 中间列优先策略
    if (isRoot) {
        int middleCol = state.N / 2;
        auto it = find(moves.begin(), moves.end(), middleCol);
        if (it != moves.end()) {
            // 将中间列移到最前面
            moves.erase(it);
            moves.insert(moves.begin(), middleCol);
        }
    }
    
    int bestMove = -1;
    
    if (maximizingPlayer) {
        int maxEval = -1e9;
        for (int move : moves) {
            GameState newState(state);
            newState.makeMove(move);
            int eval = minimax(newState, depth - 1, alpha, beta, false);
            
            if (eval > maxEval) {
                maxEval = eval;
                if (isRoot) bestMove = move;
            }
            
            alpha = max(alpha, eval);
            if (beta <= alpha) break; // Beta剪枝
        }
        
        if (isRoot) return bestMove;
        return maxEval;
    } else {
        int minEval = 1e9;
        for (int move : moves) {
            GameState newState(state);
            newState.makeMove(move);
            int eval = minimax(newState, depth - 1, alpha, beta, true);
            
            if (eval < minEval) {
                minEval = eval;
                if (isRoot) bestMove = move;
            }
            
            beta = min(beta, eval);
            if (beta <= alpha) break; // Alpha剪枝
        }
        
        if (isRoot) return bestMove;
        return minEval;
    }
}

// 使用增强的Alpha-Beta剪枝选择最佳落子点
int alphaBetaSearch(const int M, const int N, const int *top, int **board, const int lastX, const int lastY) {
    GameState state(M, N, board, top, lastX, lastY, 2); // AI是玩家2
    
    // 动态调整搜索深度，根据可用的移动数量
    int availableMoves = state.getAvailableMoves().size();
    int searchDepth = 6; // 默认深度
    
    if (availableMoves <= 5) searchDepth = 8;
    else if (availableMoves <= 3) searchDepth = 10;
    
    return minimax(state, searchDepth, -1e9, 1e9, true, true);
}

/*
    策略函数接口,该函数被对抗平台调用,每次传入当前状态,要求输出你的落子点,该落子点必须是一个符合游戏规则的落子点,不然对抗平台会直接认为你的程序有误
*/
extern "C" Point *getPoint(const int M, const int N, const int *top, const int *_board,
                           const int lastX, const int lastY, const int noX, const int noY)
{
    /*
        不要更改这段代码
    */
    int x = -1, y = -1; //最终将你的落子点存到x,y中
    int **board = new int *[M];
    for (int i = 0; i < M; i++)
    {
        board[i] = new int[N];
        for (int j = 0; j < N; j++)
        {
            board[i][j] = _board[i * N + j];
        }
    }

    /*
        根据你自己的策略来返回落子点,也就是根据你的策略完成对x,y的赋值
        该部分对参数使用没有限制，为了方便实现，你可以定义自己新的类、.h文件、.cpp文件
    */
    //Add your own code below
    
    // 初始化随机数生成器
    srand(time(nullptr));
    
    // 检查是否是第一步
    bool isFirstMove = (lastX == -1 && lastY == -1);
    
    // 检查是否是先手
    bool isFirstPlayer = isFirstMove || countPieces(M, N, board) % 2 == 0;
    
    if (isFirstMove || (isFirstPlayer && countPieces(M, N, board) <= 2)) {
        // 如果是第一步或者是先手的前几步，优先选择中间位置
        y = N / 2;
        
        // 如果中间列已满，选择最接近中间的列
        if (top[y] <= 0) {
            int offset = 1;
            while (true) {
                // 尝试中间右侧
                if (y + offset < N && top[y + offset] > 0) {
                    y = y + offset;
                    break;
                }
                // 尝试中间左侧
                if (y - offset >= 0 && top[y - offset] > 0) {
                    y = y - offset;
                    break;
                }
                offset++;
            }
        }
        
        x = top[y] - 1;
    } else {
        // 使用迭代加深的Alpha-Beta搜索
        y = alphaBetaSearch(M, N, top, board, lastX, lastY);
        
        // 如果Alpha-Beta搜索失败，使用蒙特卡洛树搜索
        if (y == -1) {
            y = monteCarloTreeSearch(M, N, top, board, lastX, lastY);
        }
        
        // 如果仍然失败，使用简单策略
        if (y == -1) {
            // 优先选择中间列及其附近的列
            int middleCol = N / 2;
            int bestCol = -1;
            int minDistance = N;
            
            for (int i = 0; i < N; i++) {
                if (top[i] > 0) {
                    int distance = abs(i - middleCol);
                    if (distance < minDistance) {
                        minDistance = distance;
                        bestCol = i;
                    }
                }
            }
            
            if (bestCol != -1) {
                y = bestCol;
            } else {
                // 如果所有策略都失败，选择第一个可用的列
                for (int i = 0; i < N; i++) {
                    if (top[i] > 0) {
                        y = i;
                        break;
                    }
                }
            }
        }
        
        x = top[y] - 1;
    }

    /*
        不要更改这段代码
    */
    clearArray(M, N, board);
    return new Point(x, y);
}

/*
    getPoint函数返回的Point指针是在本so模块中声明的，为避免产生堆错误，应在外部调用本so中的
    函数来释放空间，而不应该在外部直接delete
*/
extern "C" void clearPoint(Point *p)
{
    delete p;
    return;
}

/*
    清除top和board数组
*/
void clearArray(int M, int N, int **board)
{
    for (int i = 0; i < M; i++)
    {
        delete[] board[i];
    }
    delete[] board;
}

/*
    添加你自己的辅助函数，你可以声明自己的类、函数，添加新的.h .cpp文件来辅助实现你的想法
*/
int countPieces(const int M, const int N, int **board) {
    int count = 0;
    for (int i = 0; i < M; i++) {
        for (int j = 0; j < N; j++) {
            if (board[i][j] != 0) {
                count++;
            }
        }
    }
    return count;
}
