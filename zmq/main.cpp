#include <zmq.hpp>
#include <cstdlib>
#include <ctime>
#include <jsoncpp/json/json.h>
#include <opencv2/opencv.hpp>
#include <iostream>
#include <string>

struct Position {
    int x, y;
};

class ChaseGame {
public:
    ChaseGame() {
        reset();
    }

    void reset() {
        player_pos.x = rand() % 100;
        player_pos.y = rand() % 100;
        food_pos.x = rand() % 100;
        food_pos.y = rand() % 100;
    }

    Json::Value stepPlayer(int action) {
        // 0: 上, 1: 下, 2: 左, 3: 右
        switch(action) {
            case 0: player_pos.y = std::max(0, player_pos.y - 1); break;
            case 1: player_pos.y = std::min(99, player_pos.y + 1); break;
            case 2: player_pos.x = std::max(0, player_pos.x - 1); break;
            case 3: player_pos.x = std::min(99, player_pos.x + 1); break;
        }

        Json::Value response;
        response["player_pos"] = Json::Value(Json::arrayValue);
        response["player_pos"].append(player_pos.x);
        response["player_pos"].append(player_pos.y);
        response["food_pos"] = Json::Value(Json::arrayValue);
        response["food_pos"].append(food_pos.x);
        response["food_pos"].append(food_pos.y);
        response["reward"] = -1 * (abs(player_pos.x - food_pos.x) + abs(player_pos.y - food_pos.y));

        return response;
    }

    Json::Value stepFood(int action) {
        // 0: 上, 1: 下, 2: 左, 3: 右
        switch(action) {
            case 0: food_pos.y = std::max(0, food_pos.y - 1); break;
            case 1: food_pos.y = std::min(99, food_pos.y + 1); break;
            case 2: food_pos.x = std::max(0, food_pos.x - 1); break;
            case 3: food_pos.x = std::min(99, food_pos.x + 1); break;
        }

        Json::Value response;
        response["player_pos"] = Json::Value(Json::arrayValue);
        response["player_pos"].append(player_pos.x);
        response["player_pos"].append(player_pos.y);
        response["food_pos"] = Json::Value(Json::arrayValue);
        response["food_pos"].append(food_pos.x);
        response["food_pos"].append(food_pos.y);
        response["reward"] = 1 * (abs(player_pos.x - food_pos.x) + abs(player_pos.y - food_pos.y));

        return response;
    }

    Position getPlayerPosition() const {
        return player_pos;
    }

    Position getFoodPosition() const {
        return food_pos;
    }


    void draw() {
        cv::Mat img(1010, 1010, CV_8UC3, cv::Scalar(255, 255, 255));
        cv::rectangle(img, cv::Point(player_pos.x * 10, player_pos.y * 10), cv::Point((player_pos.x + 1) * 10, (player_pos.y + 1) * 10), cv::Scalar(0, 0, 255), -1);
        cv::rectangle(img, cv::Point(food_pos.x * 10, food_pos.y * 10), cv::Point((food_pos.x + 1) * 10, (food_pos.y + 1) * 10), cv::Scalar(0, 255, 0), -1);
        cv::imshow("Chase Game", img);
        cv::waitKey(1);
    }


private:
    Position player_pos, food_pos;
};

int main() {
    zmq::context_t context(1);
    zmq::socket_t socketPlayer(context, ZMQ_REP);
    socketPlayer.bind("tcp://*:5555");

    zmq::socket_t socketFood(context, ZMQ_REP);
    socketFood.bind("tcp://*:5556");

    ChaseGame game;
    srand(static_cast<unsigned>(time(0)));

    while (true) {
        zmq::message_t requestPlayer;
        socketPlayer.recv(requestPlayer);
        std::string req_str_player(static_cast<char*>(requestPlayer.data()), requestPlayer.size());

        zmq::message_t requestFood;
        socketFood.recv(requestFood);
        std::string req_str_food(static_cast<char*>(requestFood.data()), requestFood.size());

        Json::Value req_json_player;
        std::istringstream(req_str_player) >> req_json_player;
        int action = req_json_player["action"].asInt();

        Json::Value req_json_food;
        std::istringstream(req_str_food) >> req_json_food;
        int actionFood = req_json_food["action"].asInt();

        Json::Value responsePlayer;
        if (req_json_player["type"].asString() == "reset") {
            game.reset();
            Position player_pos = game.getPlayerPosition();
            Position food_pos = game.getFoodPosition();
            responsePlayer["player_pos"] = Json::Value(Json::arrayValue);
            responsePlayer["player_pos"].append(player_pos.x);
            responsePlayer["player_pos"].append(player_pos.y);
            responsePlayer["food_pos"] = Json::Value(Json::arrayValue);
            responsePlayer["food_pos"].append(food_pos.x);
            responsePlayer["food_pos"].append(food_pos.y);
            responsePlayer["reward"] = 0; // 重置时的奖励
        } else {
            responsePlayer = game.stepPlayer(action);
        }

        Json::Value responseFood;
        if (req_json_food["type"].asString() == "reset") {
            game.reset();
            Position player_pos = game.getPlayerPosition();
            Position food_pos = game.getFoodPosition();
            responseFood["player_pos"] = Json::Value(Json::arrayValue);
            responseFood["player_pos"].append(player_pos.x);
            responseFood["player_pos"].append(player_pos.y);
            responseFood["food_pos"] = Json::Value(Json::arrayValue);
            responseFood["food_pos"].append(food_pos.x);
            responseFood["food_pos"].append(food_pos.y);
            responseFood["reward"] = 0; // 重置时的奖励
        } else {
            responseFood = game.stepFood(actionFood);
        }

        Json::StreamWriterBuilder writerPlayer;
        std::string response_str = Json::writeString(writerPlayer, responsePlayer);

        Json::StreamWriterBuilder writerFood;
        std::string response_str_food = Json::writeString(writerFood, responseFood);

        zmq::message_t replyPlayer(response_str.size());
        memcpy(replyPlayer.data(), response_str.data(), response_str.size());
        socketPlayer.send(replyPlayer, zmq::send_flags::none);

        zmq::message_t replyFood(response_str_food.size());
        memcpy(replyFood.data(), response_str_food.data(), response_str_food.size());
        socketFood.send(replyFood, zmq::send_flags::none);

        game.draw();
        printf("Player position: (%d, %d), Food position: (%d, %d), RewardPlayer: %d, RewardFood: %d\n",
               game.getPlayerPosition().x, game.getPlayerPosition().y,
               game.getFoodPosition().x, game.getFoodPosition().y,
               responsePlayer["reward"].asInt(), responseFood["reward"].asInt());
    }

    return 0;
}
