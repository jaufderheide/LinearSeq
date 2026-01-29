#include "utils/SongJson.h"

#include <cctype>
#include <cstring>
#include <fstream>
#include <map>
#include <sstream>
#include <utility>
#include <vector>

namespace linearseq::SongJson {

namespace {

struct JsonValue {
	enum class Type { Null, Bool, Number, String, Array, Object };
	using Array = std::vector<JsonValue>;
	using Object = std::map<std::string, JsonValue>;

	Type type = Type::Null;
	double number = 0.0;
	bool boolean = false;
	std::string string;
	Array array;
	Object object;
};

class JsonParser {
public:
	explicit JsonParser(std::string input) : input_(std::move(input)), pos_(0) {}

	bool parse(JsonValue& out) {
		skipWhitespace();
		if (!parseValue(out)) {
			return false;
		}
		skipWhitespace();
		return pos_ == input_.size();
	}

private:
	void skipWhitespace() {
		while (pos_ < input_.size() && std::isspace(static_cast<unsigned char>(input_[pos_]))) {
			++pos_;
		}
	}

	bool parseValue(JsonValue& out) {
		skipWhitespace();
		if (pos_ >= input_.size()) {
			return false;
		}
		const char c = input_[pos_];
		if (c == '{') {
			return parseObject(out);
		}
		if (c == '[') {
			return parseArray(out);
		}
		if (c == '"') {
			return parseString(out);
		}
		if (c == '-' || std::isdigit(static_cast<unsigned char>(c))) {
			return parseNumber(out);
		}
		if (matchLiteral("true")) {
			out.type = JsonValue::Type::Bool;
			out.boolean = true;
			return true;
		}
		if (matchLiteral("false")) {
			out.type = JsonValue::Type::Bool;
			out.boolean = false;
			return true;
		}
		if (matchLiteral("null")) {
			out.type = JsonValue::Type::Null;
			return true;
		}
		return false;
	}

	bool parseObject(JsonValue& out) {
		if (!consume('{')) {
			return false;
		}
		out.type = JsonValue::Type::Object;
		out.object.clear();
		skipWhitespace();
		if (consume('}')) {
			return true;
		}
		while (true) {
			JsonValue keyValue;
			if (!parseString(keyValue)) {
				return false;
			}
			skipWhitespace();
			if (!consume(':')) {
				return false;
			}
			JsonValue value;
			if (!parseValue(value)) {
				return false;
			}
			out.object.emplace(keyValue.string, std::move(value));
			skipWhitespace();
			if (consume('}')) {
				break;
			}
			if (!consume(',')) {
				return false;
			}
		}
		return true;
	}

	bool parseArray(JsonValue& out) {
		if (!consume('[')) {
			return false;
		}
		out.type = JsonValue::Type::Array;
		out.array.clear();
		skipWhitespace();
		if (consume(']')) {
			return true;
		}
		while (true) {
			JsonValue value;
			if (!parseValue(value)) {
				return false;
			}
			out.array.push_back(std::move(value));
			skipWhitespace();
			if (consume(']')) {
				break;
			}
			if (!consume(',')) {
				return false;
			}
		}
		return true;
	}

	bool parseString(JsonValue& out) {
		if (!consume('"')) {
			return false;
		}
		std::string result;
		while (pos_ < input_.size()) {
			char c = input_[pos_++];
			if (c == '"') {
				out.type = JsonValue::Type::String;
				out.string = std::move(result);
				return true;
			}
			if (c == '\\') {
				if (pos_ >= input_.size()) {
					return false;
				}
				char escaped = input_[pos_++];
				switch (escaped) {
					case '"': result.push_back('"'); break;
					case '\\': result.push_back('\\'); break;
					case '/': result.push_back('/'); break;
					case 'b': result.push_back('\b'); break;
					case 'f': result.push_back('\f'); break;
					case 'n': result.push_back('\n'); break;
					case 'r': result.push_back('\r'); break;
					case 't': result.push_back('\t'); break;
					default: return false;
				}
			} else {
				result.push_back(c);
			}
		}
		return false;
	}

	bool parseNumber(JsonValue& out) {
		size_t start = pos_;
		if (input_[pos_] == '-') {
			++pos_;
		}
		while (pos_ < input_.size() && std::isdigit(static_cast<unsigned char>(input_[pos_]))) {
			++pos_;
		}
		if (pos_ < input_.size() && input_[pos_] == '.') {
			++pos_;
			while (pos_ < input_.size() && std::isdigit(static_cast<unsigned char>(input_[pos_]))) {
				++pos_;
			}
		}
		if (pos_ < input_.size() && (input_[pos_] == 'e' || input_[pos_] == 'E')) {
			++pos_;
			if (pos_ < input_.size() && (input_[pos_] == '+' || input_[pos_] == '-')) {
				++pos_;
			}
			while (pos_ < input_.size() && std::isdigit(static_cast<unsigned char>(input_[pos_]))) {
				++pos_;
			}
		}
		const std::string numStr = input_.substr(start, pos_ - start);
		char* endPtr = nullptr;
		const double value = std::strtod(numStr.c_str(), &endPtr);
		if (endPtr == numStr.c_str()) {
			return false;
		}
		out.type = JsonValue::Type::Number;
		out.number = value;
		return true;
	}

	bool matchLiteral(const char* literal) {
		size_t len = std::strlen(literal);
		if (input_.compare(pos_, len, literal) == 0) {
			pos_ += len;
			return true;
		}
		return false;
	}

	bool consume(char expected) {
		if (pos_ < input_.size() && input_[pos_] == expected) {
			++pos_;
			return true;
		}
		return false;
	}

	std::string input_;
	size_t pos_;
};

std::string escapeString(const std::string& input) {
	std::ostringstream out;
	out << '"';
	for (char c : input) {
		switch (c) {
			case '"': out << "\\\""; break;
			case '\\': out << "\\\\"; break;
			case '\b': out << "\\b"; break;
			case '\f': out << "\\f"; break;
			case '\n': out << "\\n"; break;
			case '\r': out << "\\r"; break;
			case '\t': out << "\\t"; break;
			default: out << c; break;
		}
	}
	out << '"';
	return out.str();
}

std::string statusToString(MidiStatus status) {
	switch (status) {
		case MidiStatus::NoteOff: return "NoteOff";
		case MidiStatus::NoteOn: return "NoteOn";
		case MidiStatus::PolyAftertouch: return "PolyAftertouch";
		case MidiStatus::ControlChange: return "ControlChange";
		case MidiStatus::ProgramChange: return "ProgramChange";
		case MidiStatus::ChannelAftertouch: return "ChannelAftertouch";
		case MidiStatus::PitchBend: return "PitchBend";
		default: return "NoteOn";
	}
}

bool stringToStatus(const std::string& value, MidiStatus& status) {
	if (value == "NoteOff") { status = MidiStatus::NoteOff; return true; }
	if (value == "NoteOn") { status = MidiStatus::NoteOn; return true; }
	if (value == "PolyAftertouch") { status = MidiStatus::PolyAftertouch; return true; }
	if (value == "ControlChange") { status = MidiStatus::ControlChange; return true; }
	if (value == "ProgramChange") { status = MidiStatus::ProgramChange; return true; }
	if (value == "ChannelAftertouch") { status = MidiStatus::ChannelAftertouch; return true; }
	if (value == "PitchBend") { status = MidiStatus::PitchBend; return true; }
	return false;
}

bool getNumber(const JsonValue::Object& obj, const std::string& key, double& out) {
	auto it = obj.find(key);
	if (it == obj.end() || it->second.type != JsonValue::Type::Number) {
		return false;
	}
	out = it->second.number;
	return true;
}

bool getString(const JsonValue::Object& obj, const std::string& key, std::string& out) {
	auto it = obj.find(key);
	if (it == obj.end() || it->second.type != JsonValue::Type::String) {
		return false;
	}
	out = it->second.string;
	return true;
}

bool getArray(const JsonValue::Object& obj, const std::string& key, JsonValue::Array& out) {
	auto it = obj.find(key);
	if (it == obj.end() || it->second.type != JsonValue::Type::Array) {
		return false;
	}
	out = it->second.array;
	return true;
}

} // namespace

std::string toJson(const Song& song) {
	std::ostringstream out;
	out << "{";
	out << "\"ppqn\":" << song.ppqn << ",";
	out << "\"bpm\":" << song.bpm << ",";
	out << "\"midiDevice\":" << escapeString(song.midiDevice) << ",";
	out << "\"tracks\":[";
	for (size_t t = 0; t < song.tracks.size(); ++t) {
		const auto& track = song.tracks[t];
		if (t > 0) {
			out << ",";
		}
		out << "{";
		out << "\"name\":" << escapeString(track.name) << ",";
		out << "\"alsaClient\":" << track.alsaClient << ",";
		out << "\"alsaPort\":" << track.alsaPort << ",";
		out << "\"channel\":" << static_cast<int>(track.channel) << ",";
		out << "\"items\":[";
		for (size_t i = 0; i < track.items.size(); ++i) {
			const auto& item = track.items[i];
			if (i > 0) {
				out << ",";
			}
			out << "{";
			out << "\"startTick\":" << item.startTick << ",";
			out << "\"lengthTicks\":" << item.lengthTicks << ",";
			out << "\"events\":[";
			for (size_t e = 0; e < item.events.size(); ++e) {
				const auto& ev = item.events[e];
				if (e > 0) {
					out << ",";
				}
				out << "{";
				out << "\"tick\":" << ev.tick << ",";
				out << "\"status\":" << escapeString(statusToString(ev.status)) << ",";
				out << "\"channel\":" << static_cast<int>(ev.channel) << ",";
				out << "\"data1\":" << static_cast<int>(ev.data1) << ",";
				out << "\"data2\":" << static_cast<int>(ev.data2) << ",";
				out << "\"duration\":" << ev.duration;
				out << "}";
			}
			out << "]";
			out << "}";
		}
		out << "]";
		out << "}";
	}
	out << "]";
	out << "}";
	return out.str();
}

bool saveToFile(const Song& song, const std::string& path) {
	std::ofstream file(path, std::ios::binary | std::ios::trunc);
	if (!file) {
		return false;
	}
	file << toJson(song);
	return static_cast<bool>(file);
}

bool loadFromFile(const std::string& path, Song& song) {
	std::ifstream file(path, std::ios::binary);
	if (!file) {
		return false;
	}
	std::ostringstream buffer;
	buffer << file.rdbuf();
	JsonParser parser(buffer.str());
	JsonValue root;
	if (!parser.parse(root) || root.type != JsonValue::Type::Object) {
		return false;
	}
	const auto& obj = root.object;
	double ppqnValue = 0.0;
	double bpmValue = 0.0;
	JsonValue::Array tracksArray;
	if (!getNumber(obj, "ppqn", ppqnValue)) {
		return false;
	}
	if (!getNumber(obj, "bpm", bpmValue)) {
		return false;
	}
	if (!getArray(obj, "tracks", tracksArray)) {
		return false;
	}

	Song loaded;
	loaded.ppqn = static_cast<uint32_t>(ppqnValue);
	loaded.bpm = bpmValue;
	std::string midiDevice;
	if (getString(obj, "midiDevice", midiDevice)) {
		loaded.midiDevice = midiDevice;
	}
	for (const auto& trackValue : tracksArray) {
		if (trackValue.type != JsonValue::Type::Object) {
			return false;
		}
		const auto& trackObj = trackValue.object;
		Track track;
		std::string name;
		double alsaClient = -1;
		double alsaPort = -1;
		double channel = 0;
		JsonValue::Array itemsArray;
		if (!getString(trackObj, "name", name)) {
			name = "Track";
		}
		if (!getNumber(trackObj, "alsaClient", alsaClient)) {
			alsaClient = -1;
		}
		if (!getNumber(trackObj, "alsaPort", alsaPort)) {
			alsaPort = -1;
		}
		if (!getNumber(trackObj, "channel", channel)) {
			channel = 0;
		}
		if (!getArray(trackObj, "items", itemsArray)) {
			return false;
		}
		track.name = name;
		track.alsaClient = static_cast<int>(alsaClient);
		track.alsaPort = static_cast<int>(alsaPort);
		track.channel = static_cast<uint8_t>(channel);
		for (const auto& itemValue : itemsArray) {
			if (itemValue.type != JsonValue::Type::Object) {
				return false;
			}
			const auto& itemObj = itemValue.object;
			double startTick = 0.0;
			double lengthTicks = 0.0;
			JsonValue::Array eventsArray;
			if (!getNumber(itemObj, "startTick", startTick)) {
				return false;
			}
			if (!getNumber(itemObj, "lengthTicks", lengthTicks)) {
				return false;
			}
			if (!getArray(itemObj, "events", eventsArray)) {
				return false;
			}
			MidiItem item;
			item.startTick = static_cast<uint32_t>(startTick);
			item.lengthTicks = static_cast<uint32_t>(lengthTicks);
			for (const auto& eventValue : eventsArray) {
				if (eventValue.type != JsonValue::Type::Object) {
					return false;
				}
				const auto& eventObj = eventValue.object;
				double tick = 0.0;
				double channelValue = 0.0;
				double data1 = 0.0;
				double data2 = 0.0;
				double duration = 0.0;
				std::string statusString;
				if (!getNumber(eventObj, "tick", tick)) {
					return false;
				}
				if (!getString(eventObj, "status", statusString)) {
					return false;
				}
				if (!getNumber(eventObj, "channel", channelValue)) {
					return false;
				}
				if (!getNumber(eventObj, "data1", data1)) {
					return false;
				}
				if (!getNumber(eventObj, "data2", data2)) {
					return false;
				}
				if (!getNumber(eventObj, "duration", duration)) {
					return false;
				}
				MidiStatus status;
				if (!stringToStatus(statusString, status)) {
					return false;
				}
				MidiEvent event;
				event.tick = static_cast<uint32_t>(tick);
				event.status = status;
				event.channel = static_cast<uint8_t>(channelValue);
				event.data1 = static_cast<uint8_t>(data1);
				event.data2 = static_cast<uint8_t>(data2);
				event.duration = static_cast<uint32_t>(duration);
				item.events.push_back(event);
			}
			track.items.push_back(item);
		}
		loaded.tracks.push_back(track);
	}

	song = std::move(loaded);
	return true;
}

} // namespace linearseq::SongJson
