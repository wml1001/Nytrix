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
        bool selected = false;  // ѡ��״̬
    };

    //�洢�������ṹ��
    struct  ListenerItem
    {
        std::string name;
        std::string payload;
        std::string ip;
        std::string port;
    };

    const float SPLITTER_HEIGHT = 5.0f;        // �ָ����߶�
    const float MIN_VIEW_HEIGHT = 50.0f;       // ��ͼ��С�߶�
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