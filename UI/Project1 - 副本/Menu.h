#pragma once
//Menu.h
#include<unordered_map>
#include<string>

#include "Listener.h"
#include "Cmd.h"

namespace App {

    struct ContentItem {
        std::string id;
        std::string ip;
        std::string note = "DefaultNote";
        std::string user = "DefaultUser";
        std::string computer = "DefaultPC";
        bool selected = false;  // 选中状态
    };

    //存储监听器结构体
    struct  ListenerItem
    {
        std::string name;
        std::string payload;
        std::string ip;
        std::string port;
    };

    const float SPLITTER_HEIGHT = 5.0f;        // 分隔条高度
    const float MIN_VIEW_HEIGHT = 50.0f;       // 视图最小高度
    const int CMD_SIZE = 4096;

	std::string GbkToUtf8(std::string& gbkStr);
	void RenderUI(AuthenticationManager* authenticationManager);
    void parseString(const std::string& input);
    std::unordered_map<std::string, bool>& getWindowsStates();
    std::vector<ContentItem>& getContentItems();
    std::vector<ListenerItem>& getListenerItems();
    std::vector<ListenerItem>& getSelectListenerItem();
    float& getSplitterRatio();
    bool& getDraggingSplitter();
}

void hello();