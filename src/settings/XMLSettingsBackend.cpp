/*
 * MIT License
 *
 * Copyright (c) 2026 Joey Castillo
 *
 * Permission is hereby granted, free of charge, to any person obtaining a copy
 * of this software and associated documentation files (the "Software"), to deal
 * in the Software without restriction, including without limitation the rights
 * to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
 * copies of the Software, and to permit persons to whom the Software is
 * furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included in all
 * copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
 * AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
 * OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
 * SOFTWARE.
 */

#include "XMLSettingsBackend.hpp"
#include "tinyxml2.h"

using namespace tinyxml2;

XMLSettingsBackend::XMLSettingsBackend(const std::string& filePath)
    : filePath(filePath), doc(std::make_unique<XMLDocument>()) {
    if (doc->LoadFile(filePath.c_str()) != XML_SUCCESS) {
        doc->Clear();
        doc->InsertFirstChild(doc->NewElement("settings"));
    }
}

XMLSettingsBackend::~XMLSettingsBackend() = default;

XMLElement* XMLSettingsBackend::findElement(const std::string& key) {
    XMLElement* root = doc->RootElement();
    if (!root) return nullptr;

    for (XMLElement* el = root->FirstChildElement(); el; el = el->NextSiblingElement()) {
        const char* k = el->Attribute("key");
        if (k && key == k) {
            return el;
        }
    }
    return nullptr;
}

XMLElement* XMLSettingsBackend::findOrCreateElement(const std::string& key, const char* typeName) {
    XMLElement* existing = findElement(key);
    if (existing) {
        // If the type changed, update the element name
        if (strcmp(existing->Name(), typeName) != 0) {
            existing->SetName(typeName);
        }
        return existing;
    }

    XMLElement* root = doc->RootElement();
    XMLElement* el = root->InsertNewChildElement(typeName);
    el->SetAttribute("key", key.c_str());
    return el;
}

void XMLSettingsBackend::save() {
    doc->SaveFile(filePath.c_str());
}

bool XMLSettingsBackend::hasKey(const std::string& key) {
    return findElement(key) != nullptr;
}

void XMLSettingsBackend::eraseKey(const std::string& key) {
    XMLElement* el = findElement(key);
    if (el) {
        doc->RootElement()->DeleteChild(el);
        save();
    }
}

std::string XMLSettingsBackend::getString(const std::string& key) {
    XMLElement* el = findElement(key);
    if (!el) return "";
    const char* text = el->GetText();
    return text ? text : "";
}

void XMLSettingsBackend::setString(const std::string& key, const std::string& value) {
    XMLElement* el = findOrCreateElement(key, "string");
    el->SetText(value.c_str());
    save();
}

int32_t XMLSettingsBackend::getInt(const std::string& key) {
    XMLElement* el = findElement(key);
    if (!el) return 0;
    return el->IntText(0);
}

void XMLSettingsBackend::setInt(const std::string& key, int32_t value) {
    XMLElement* el = findOrCreateElement(key, "int");
    el->SetText(static_cast<int>(value));
    save();
}

bool XMLSettingsBackend::getBool(const std::string& key) {
    XMLElement* el = findElement(key);
    if (!el) return false;
    return el->BoolText(false);
}

void XMLSettingsBackend::setBool(const std::string& key, bool value) {
    XMLElement* el = findOrCreateElement(key, "bool");
    el->SetText(value);
    save();
}
