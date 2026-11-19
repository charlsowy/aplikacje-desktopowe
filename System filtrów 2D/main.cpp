#include <SFML/Graphics.hpp>
#include "imgui.h"
#include "imgui-SFML.h"
#include <windows.h>
#include <iostream>
#include <string>
#include <array>
#include <optional>
#include <algorithm>


struct Filters {
    bool negative = false;
    bool grayscale = false;
    bool brightness = false;
    bool blur = false;
    bool sobel = false;
    float brightnessValue = 0.0f;
    float contrastValue = 1.0f;
    float blurRadius = 2.0f;
    float sobelThreshold = 0.5f;
};


struct Shaders {
    sf::Shader negative, grayscale, bc, blur, sobel;
};

const sf::Texture& applyShader(const sf::Texture& in,
    std::array<sf::RenderTexture, 2>& rts,
    sf::Shader& shader) {
    sf::RenderTexture& out = (&in == &rts[0].getTexture()) ? rts[1] : rts[0];
    out.clear(sf::Color::Transparent);
    out.draw(sf::Sprite(in), &shader);
    out.display();
    return out.getTexture();
}

const sf::Texture& runPipeline(const Filters& f, const sf::Texture& image,
    std::array<sf::RenderTexture, 2>& rts, Shaders& sh) {
    sf::Vector2f size(image.getSize());
    const sf::Texture* cur = &image;

    if (f.negative)  cur = &applyShader(*cur, rts, sh.negative);
    if (f.grayscale) cur = &applyShader(*cur, rts, sh.grayscale);
    if (f.brightness) {
        sh.bc.setUniform("brightness", f.brightnessValue);
        sh.bc.setUniform("contrast", f.contrastValue);
        cur = &applyShader(*cur, rts, sh.bc);
    }
    if (f.blur) {
        sh.blur.setUniform("blurRadius", f.blurRadius);
        sh.blur.setUniform("texSize", size);
        cur = &applyShader(*cur, rts, sh.blur);
    }
    if (f.sobel) {
        sh.sobel.setUniform("threshold", f.sobelThreshold);
        sh.sobel.setUniform("texSize", size);
        cur = &applyShader(*cur, rts, sh.sobel);
    }
    return *cur;
}

int main() {
    sf::RenderWindow window(sf::VideoMode({ 1000, 800 }), "2D Filter System");
    window.setFramerateLimit(60);

    if (!ImGui::SFML::Init(window)) {
        return -1;
    }

    sf::Texture imageTexture;
    std::optional<sf::Sprite> imageSprite;
    bool isImageLoaded = false;
    std::string currentFilePath = "";
    std::string popupMessage = "";
    bool showPopup = false;


    bool showBCWindow = false;
    bool showNegativeWindow = false;
    bool showGrayscaleWindow = false;
    bool showBlurWindow = false;
    bool showSobelWindow = false;

    Filters applied;


    bool tmpNegative = false;
    bool tmpGrayscale = false;
    bool tmpBC = false;
    float tmpBrightness = 0.0f;
    float tmpContrast = 1.0f;
    bool tmpBlur = false;
    float tmpBlurRadius = 2.0f;
    bool tmpSobel = false;
    float tmpSobelThreshold = 0.5f;


    Shaders sh;
    if (!sh.negative.loadFromFile("negative.frag", sf::Shader::Type::Fragment))
        std::cerr << "ERROR: Failed to load negative.frag shader!" << std::endl;
    if (!sh.grayscale.loadFromFile("grayscale.frag", sf::Shader::Type::Fragment))
        std::cerr << "ERROR: Failed to load grayscale.frag shader!" << std::endl;
    if (!sh.blur.loadFromFile("blur.frag", sf::Shader::Type::Fragment))
        std::cerr << "ERROR: Failed to load blur.frag shader!" << std::endl;
    if (!sh.sobel.loadFromFile("sobel.frag", sf::Shader::Type::Fragment))
        std::cerr << "ERROR: Failed to load sobel.frag shader!" << std::endl;
    if (!sh.bc.loadFromFile("brightness_contrast.frag", sf::Shader::Type::Fragment))
        std::cerr << "ERROR: Failed to load brightness_contrast.frag shader!" << std::endl;

    sh.negative.setUniform("texture", sf::Shader::CurrentTexture);
    sh.grayscale.setUniform("texture", sf::Shader::CurrentTexture);
    sh.blur.setUniform("texture", sf::Shader::CurrentTexture);
    sh.sobel.setUniform("texture", sf::Shader::CurrentTexture);
    sh.bc.setUniform("texture", sf::Shader::CurrentTexture);

    std::array<sf::RenderTexture, 2> rts;

    auto saveImage = [&](const std::string& path) -> bool {
        const sf::Texture& result = runPipeline(applied, imageTexture, rts, sh);
        sf::Image img = result.copyToImage();
        return img.saveToFile(path);
        };

    sf::Clock deltaClock;
    while (window.isOpen()) {
        while (const std::optional event = window.pollEvent()) {
            ImGui::SFML::ProcessEvent(window, *event);
            if (event->is<sf::Event::Closed>()) {
                window.close();
            }
        }

        ImGui::SFML::Update(window, deltaClock.restart());

        ImGui::PushStyleColor(ImGuiCol_MenuBarBg, ImVec4(0.25f, 0.25f, 0.30f, 1.0f));
        ImGui::PushStyleColor(ImGuiCol_PopupBg, ImVec4(0.22f, 0.22f, 0.28f, 1.0f));
        ImGui::PushStyleColor(ImGuiCol_HeaderHovered, ImVec4(0.20f, 0.55f, 1.00f, 1.0f));

        if (ImGui::BeginMainMenuBar()) {
            if (ImGui::BeginMenu("File")) {
                if (ImGui::MenuItem("Open")) {
                    OPENFILENAMEA ofn;
                    char szFile[260] = { 0 };

                    ZeroMemory(&ofn, sizeof(ofn));
                    ofn.lStructSize = sizeof(ofn);
                    ofn.hwndOwner = NULL;
                    ofn.lpstrFile = szFile;
                    ofn.nMaxFile = sizeof(szFile);
                    ofn.lpstrFilter = "Image Files (*.png; *.jpg)\0*.png;*.jpg\0All Files (*.*)\0*.*\0";
                    ofn.nFilterIndex = 1;
                    ofn.Flags = OFN_PATHMUSTEXIST | OFN_FILEMUSTEXIST;

                    if (GetOpenFileNameA(&ofn) == TRUE) {
                        if (imageTexture.loadFromFile(szFile)) {
                            imageSprite.emplace(imageTexture);
                            isImageLoaded = true;
                            currentFilePath = szFile;

                            sf::Vector2u texSize = imageTexture.getSize();
                            rts[0].resize(texSize);
                            rts[1].resize(texSize);

                            float maxW = 900.0f;
                            float maxH = 700.0f;
                            float scale = std::min(maxW / texSize.x, maxH / texSize.y);
                            imageSprite->setScale({ scale, scale });

                            float spriteWidth = texSize.x * scale;
                            float spriteHeight = texSize.y * scale;
                            imageSprite->setPosition({
                                (1000.0f - spriteWidth) / 2.0f,
                                (800.0f - spriteHeight) / 2.0f + 20.0f
                                });
                        }
                        else {
                            popupMessage = "ERROR: Failed to load file as an image!";
                            showPopup = true;
                        }
                    }
                }
                if (ImGui::MenuItem("Save")) {
                    if (isImageLoaded && !currentFilePath.empty()) {
                        if (saveImage(currentFilePath)) {
                            popupMessage = "SUCCESS: Image saved successfully!";
                        }
                        else {
                            popupMessage = "ERROR: Failed to save file!";
                        }
                        showPopup = true;
                    }
                }
                if (ImGui::MenuItem("Save as...")) {
                    OPENFILENAMEA ofn;
                    char szFile[260] = { 0 };

                    ZeroMemory(&ofn, sizeof(ofn));
                    ofn.lStructSize = sizeof(ofn);
                    ofn.hwndOwner = NULL;
                    ofn.lpstrFile = szFile;
                    ofn.nMaxFile = sizeof(szFile);
                    ofn.lpstrFilter = "PNG Image (*.png)\0*.png\0JPG Image (*.jpg)\0*.jpg\0All Files (*.*)\0*.*\0";
                    ofn.nFilterIndex = 1;
                    ofn.lpstrDefExt = "png";
                    ofn.Flags = OFN_PATHMUSTEXIST | OFN_OVERWRITEPROMPT;

                    if (GetSaveFileNameA(&ofn) == TRUE && isImageLoaded) {
                        if (saveImage(szFile)) {
                            popupMessage = "SUCCESS: Image saved successfully!";
                            currentFilePath = szFile;
                        }
                        else {
                            popupMessage = "ERROR: Failed to save file under the specified path!";
                        }
                        showPopup = true;
                    }
                }
                ImGui::EndMenu();
            }
            if (ImGui::MenuItem("Brightness & Contrast")) {
                showBCWindow = true;
                tmpBC = true;
                tmpBrightness = applied.brightnessValue;
                tmpContrast = applied.contrastValue;
            }
            if (ImGui::MenuItem("Negative...")) {
                showNegativeWindow = true;
                tmpNegative = applied.negative;
            }
            if (ImGui::MenuItem("Grayscale...")) {
                showGrayscaleWindow = true;
                tmpGrayscale = applied.grayscale;
            }
            if (ImGui::MenuItem("Box Blur")) {
                showBlurWindow = true;
                tmpBlur = true;
                tmpBlurRadius = applied.blurRadius;
            }
            if (ImGui::MenuItem("Sobel Filter")) {
                showSobelWindow = true;
                tmpSobel = true;
                tmpSobelThreshold = applied.sobelThreshold;
            }
            ImGui::EndMainMenuBar();
        }

        ImGui::PopStyleColor(3);
        if (showBCWindow) {
            ImGui::Begin("Image Adjustments", &showBCWindow, ImGuiWindowFlags_AlwaysAutoResize);
            ImGui::SliderFloat("Brightness", &tmpBrightness, -1.0f, 1.0f);
            ImGui::SliderFloat("Contrast", &tmpContrast, 0.0f, 3.0f);
            ImGui::Spacing(); ImGui::Separator(); ImGui::Spacing();

            if (ImGui::Button("Apply", ImVec2(100, 0))) {
                applied.brightness = tmpBC;
                applied.brightnessValue = tmpBrightness;
                applied.contrastValue = tmpContrast;
                showBCWindow = false;
            }
            ImGui::SameLine();
            if (ImGui::Button("Cancel", ImVec2(100, 0))) {
                showBCWindow = false;
            }
            ImGui::End();
        }

        if (showNegativeWindow) {
            ImGui::Begin("Negative Settings", &showNegativeWindow, ImGuiWindowFlags_AlwaysAutoResize);
            ImGui::Checkbox("Enable Negative", &tmpNegative);
            ImGui::Spacing(); ImGui::Separator(); ImGui::Spacing();

            if (ImGui::Button("Apply", ImVec2(100, 0))) {
                applied.negative = tmpNegative;
                showNegativeWindow = false;
            }
            ImGui::SameLine();
            if (ImGui::Button("Cancel", ImVec2(100, 0))) {
                showNegativeWindow = false;
            }
            ImGui::End();
        }

        if (showGrayscaleWindow) {
            ImGui::Begin("Grayscale Settings", &showGrayscaleWindow, ImGuiWindowFlags_AlwaysAutoResize);
            ImGui::Checkbox("Enable Grayscale", &tmpGrayscale);
            ImGui::Spacing(); ImGui::Separator(); ImGui::Spacing();

            if (ImGui::Button("Apply", ImVec2(100, 0))) {
                applied.grayscale = tmpGrayscale;
                showGrayscaleWindow = false;
            }
            ImGui::SameLine();
            if (ImGui::Button("Cancel", ImVec2(100, 0))) {
                showGrayscaleWindow = false;
            }
            ImGui::End();
        }

        if (showBlurWindow) {
            ImGui::Begin("Box Blur Settings", &showBlurWindow, ImGuiWindowFlags_AlwaysAutoResize);
            ImGui::SliderFloat("Blur Radius", &tmpBlurRadius, 0.0f, 10.0f);
            ImGui::Spacing(); ImGui::Separator(); ImGui::Spacing();

            if (ImGui::Button("Apply", ImVec2(100, 0))) {
                applied.blur = tmpBlur;
                applied.blurRadius = tmpBlurRadius;
                showBlurWindow = false;
            }
            ImGui::SameLine();
            if (ImGui::Button("Cancel", ImVec2(100, 0))) {
                showBlurWindow = false;
            }
            ImGui::End();
        }

        if (showSobelWindow) {
            ImGui::Begin("Sobel Settings", &showSobelWindow, ImGuiWindowFlags_AlwaysAutoResize);
            ImGui::SliderFloat("Threshold", &tmpSobelThreshold, 0.0f, 1.0f);
            ImGui::Spacing(); ImGui::Separator(); ImGui::Spacing();

            if (ImGui::Button("Apply", ImVec2(100, 0))) {
                applied.sobel = tmpSobel;
                applied.sobelThreshold = tmpSobelThreshold;
                showSobelWindow = false;
            }
            ImGui::SameLine();
            if (ImGui::Button("Cancel", ImVec2(100, 0))) {
                showSobelWindow = false;
            }
            ImGui::End();
        }

        if (showPopup) {
            ImGui::OpenPopup("Notification");
            showPopup = false;
        }
        if (ImGui::BeginPopupModal("Notification", NULL, ImGuiWindowFlags_AlwaysAutoResize)) {
            ImGui::Text("%s", popupMessage.c_str());
            ImGui::Spacing();
            if (ImGui::Button("OK", ImVec2(120, 0))) {
                ImGui::CloseCurrentPopup();
            }
            ImGui::EndPopup();
        }

        window.clear(sf::Color(30, 30, 30));

        if (isImageLoaded && imageSprite.has_value()) {
            Filters preview = applied;
            if (showNegativeWindow)  preview.negative = tmpNegative;
            if (showGrayscaleWindow) preview.grayscale = tmpGrayscale;
            if (showBCWindow) {
                preview.brightness = tmpBC;
                preview.brightnessValue = tmpBrightness;
                preview.contrastValue = tmpContrast;
            }
            if (showBlurWindow) {
                preview.blur = tmpBlur;
                preview.blurRadius = tmpBlurRadius;
            }
            if (showSobelWindow) {
                preview.sobel = tmpSobel;
                preview.sobelThreshold = tmpSobelThreshold;
            }
            const sf::Texture& result = runPipeline(preview, imageTexture, rts, sh);

            sf::Sprite finalSprite(result);
            finalSprite.setScale(imageSprite->getScale());
            finalSprite.setPosition(imageSprite->getPosition());
            window.draw(finalSprite);
        }

        ImGui::SFML::Render(window);
        window.resetGLStates();
        window.display();
    }

    ImGui::SFML::Shutdown();
    return 0;
}