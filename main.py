import zmq
import json
import gym
from gym import spaces
import numpy as np
from stable_baselines3 import PPO

context = zmq.Context()
socket = context.socket(zmq.REQ)
socket.connect("tcp://localhost:5555")

def send_message(action, type="step"):
    # Convert action to int before sending
    action = int(action)
    socket.send_json({"action": action, "type": type})
    message = socket.recv_json()
    return message

class ChaseEnv(gym.Env):
    def __init__(self):
        super(ChaseEnv, self).__init__()
        self.action_space = spaces.Discrete(4) # 上、下、左、右
        self.observation_space = spaces.Box(low=0, high=99, shape=(4,), dtype=int)

    def step(self, action):
        response = send_message(action)
        state = np.array([response["player_pos"][0], response["player_pos"][1],
                          response["food_pos"][0], response["food_pos"][1]])
        reward = response["reward"]
        done = reward == 0 # 如果玩家和食物的位置重合，则游戏结束
        info = {}
        return state, reward, done, info

    def reset(self):
        response = send_message(0, "reset")  # 动作0，但是请求类型为reset
        state = np.array([response["player_pos"][0], response["player_pos"][1],
                          response["food_pos"][0], response["food_pos"][1]])
        return state

def train_model(env):
    model = PPO("MlpPolicy", env, verbose=1)  # 使用多层感知机作为策略网络
    model.learn(total_timesteps=1000000)  # 训练10000步
    return model

# Test the trained model
def test_model(model, env, episodes=1000):
    for episode in range(episodes):
        state = env.reset()
        total_reward = 0
        done = False
        while not done:
            action, _ = model.predict(state)  # 使用训练好的模型预测动作
            next_state, reward, done, _ = env.step(action)
            total_reward += reward
            state = next_state
        print(f"Episode {episode + 1}, Total Reward: {total_reward}")


if __name__ == "__main__":
    env = ChaseEnv()
    model = train_model(env)
    test_model(model, env)