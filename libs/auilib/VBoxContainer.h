#pragma once

#include <4dm.h>

using namespace fdm;

namespace aui {
	//Dogshit code, dont use it pls for your own sake
	class VBoxContainer : public gui::Element, public gui::ElemContainer
	{
	public:
		int currentCursorType = GLFW_ARROW_CURSOR;
		gui::Element* selectedElem;
		std::vector<gui::Element*> elements{};
		QuadRenderer* background = Item::qr;
		uint32_t width = 0;
		uint32_t height = 0;

		unsigned int maxColumns = 1;

		int xPos = 0;
		int yPos = 0;

		int xOffset = 0;
		int yOffset = 0;

		int elementXOffset = 0;
		int elementYOffset = 0;

		int xMargin = 10;
		int yMargin = 10;

		int xPadding = 8;
		int yPadding = 8;

		int xSpacing = 0;
		int ySpacing = 0;

		gui::AlignmentX xAlign = gui::ALIGN_LEFT;
		gui::AlignmentY yAlign = gui::ALIGN_TOP;

		bool renderBackground = false;

		void render(gui::Window* w) override
		{
			if (elements.size() < 1) return;
			int Height = 0;
			int Width = 0;
			width = 0;
			height = 0;
			//Get size of the box based on child elements
			for (int i = 0;i < elements.size();i++) {
				elements[i]->getSize(w, &Width, &Height);

				//All characters in the font are 1 pixel higher, so have to manually calculate height of a text
				//Hope 0.3 font system will fix that 
				if (auto* text = dynamic_cast<gui::Text*>(elements[i]))
					Height = text->size * 7;

				if (Width > width) width = Width;
				height += Height + ySpacing;
			}

			w->getSize(&Width, &Height);
			width += xMargin*2;
			height += yMargin * 2;

			int columns = std::min(Width / (width+xSpacing), maxColumns);
			int rows = (elements.size() + columns - 1) / columns;
			// set own position
			switch (xAlign) {
			case gui::ALIGN_CENTER_X:
				xPos=(Width / 2 - width / 2+ xOffset);
				break;
			case gui::ALIGN_LEFT:
				xPos = (xOffset);
				break;
			case gui::ALIGN_RIGHT:
				xPos = (Width - width-xPadding+ xOffset);
				break;
			};

			switch (yAlign) {
			case gui::ALIGN_CENTER_Y:
				yPos = (Height / 2 - height / 2+yOffset);
				break;
			case gui::ALIGN_TOP:
				yPos = (0 + yOffset);
				break;
			case gui::ALIGN_BOTTOM:
				yPos = (Height - height-yPadding + yOffset);
				break;
			};


			//Arrange child elements
			int posX = xPos + elementXOffset + xMargin-(columns-1)*(width/2+xSpacing);
			for (int j = 0;j < columns;j++) {
				int posY = yPos + elementYOffset + yMargin;
				for (int i = 0;i < rows;i++) {
					int index = i * columns + j;
					if (index >= elements.size()) break;
					elements[index]->getSize(w, &Width, &Height);

					if (auto* text = dynamic_cast<gui::Text*>(elements[index]))
						Height = text->size * 7;
					elements[index]->offsetY(posY );
					//elements[i]->offsetX(posX + width / 2 - Width / 2); //alligns all elements to right
					elements[index]->offsetX(posX ); //alligns all elements to center
					//elements[i]->offsetX(posX - width / 2 + Width / 2); //alligns all elements to left
					
					posY += Height + ySpacing;
				}
				posX += width+xSpacing;
			}
			
			//Render stuff
			if (renderBackground) {
				background->setPos(xPos+xOffset + xPadding,yPos+ yOffset + yPadding, width - xPadding * 2, height - yPadding * 2);
				background->setQuadRendererMode(GL_TRIANGLES);
				background->setColor(0, 0, 0, 0.5);
				background->render();

				background->setQuadRendererMode(GL_LINE_LOOP);
				background->setColor(1, 1, 1, 0.5);
				background->render();
			}
			
			for (int i = 0;i < elements.size();i++) {
				elements[i]->render(w);
			}
			
		}

		void alignX(gui::AlignmentX alignment) override { xAlign = alignment; }
		void alignY(gui::AlignmentY alignment) override { yAlign = alignment; }
		void offsetX(int offset) override { xOffset = offset; }
		void offsetY(int offset) override { yOffset = offset; }
		void getSize(const gui::Window* w, int* x, int* y) const override { *x = width; *y = height; }
		void getPos(const gui::Window* w, int* x, int* y) const override { *x = xPos; *y = yPos; }

		void addElement(gui::Element* e) override
		{
			elements.push_back(e);
		}
		void addElement(gui::Element* e, int pos)
		{
			elements.insert(elements.begin() + pos, e);
		}
		bool removeElement(gui::Element* e) override
		{
			return elements.erase(std::remove(elements.begin(), elements.end(), e), elements.end()) != elements.end();
		}
		void clear() override
		{
			elements.clear();
		}
		bool empty() override
		{
			return elements.empty();
		}

		//Succesfully stolen from ContentBox.h


		bool mouseInput(const gui::Window* w, double xpos, double ypos) override
		{
			if (elements.empty()) return false;
			
			for (auto& el : elements)
			{
				bool r = el->mouseInput(w, xpos, ypos);
				if (r)
				{
					this->currentCursorType = el->getCursorType();
					return true;
				}
			}

			return false;
		}
		bool mouseButtonInput(const gui::Window* w, int button, int action, int mods) override
		{
			if (elements.empty()) return false;

			for (auto& el : elements)
			{
				bool r = el->mouseButtonInput(w, button, action, mods);
				if (r)
				{
					this->currentCursorType = el->getCursorType();
					selectedElem = el;
					return true;
				}
			}
			
		}
		void select() override 
		{ 
			if (elements.empty()) return;
			if (selectedElem == nullptr) return;
			selectedElem->select();
		}
		void deselect() override 
		{ 
			if (elements.empty()) return;
			if (selectedElem == nullptr) return;
			selectedElem->deselect();
			selectedElem = nullptr;
		}
	};

	
}