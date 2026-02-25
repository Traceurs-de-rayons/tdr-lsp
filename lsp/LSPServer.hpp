#pragma once

#include <iostream>
#include <string>
#include <map>
#include "tdr/LanguageService.hpp"
#include "tdr/SceneSchema.hpp"
#include "tdr/parser.hpp"
#include "json.hpp"

using json = nlohmann::json;
using namespace sceneIO::tdr;

struct ColorInfo
{
	double r, g, b;
};

class LSPServer
{
public:
	void run();

private:
	std::map<std::string, std::string> open_documents;
	SceneLanguageService service_;
	SceneSchema sch;

	std::string get_document_text(const std::string& uri);
	void handle_message(const std::string& content);
	void handle_initialize(const json& msg);
	void handle_did_open(const json& msg);
	void handle_did_change(const json& msg);
	void handle_hover(const json& msg);
	void handle_document_color(const json& msg);
	void handle_color_presentation(const json& msg);
	void publish_diagnostics(const std::string& uri, const std::string& text);
	void send_response(const json& id, const json& result);
	void send_notification(const std::string& method, const json& params);
	void send_json(const json& msg);
	void find_colors_in_ast(const Node& ast, json& colors);
	json node_text_range(const Node& node);
};
