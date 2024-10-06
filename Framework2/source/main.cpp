/*
Copyright 2020 Alejandro Cosin

This program is free software: you can redistribute it and/or modify
it under the terms of the GNU General Public License as published by
the Free Software Foundation, either version 3 of the License, or
(at your option) any later version.

This program is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
GNU General Public License for more details.

You should have received a copy of the GNU General Public License
along with this program.  If not, see <http://www.gnu.org/licenses/>.
*/

// GLOBAL INCLUDES
#include <chrono>

// PROJECT INCLUDES
#include "../include/headers.h"
#include "../include/core/coremanager.h"
#include "../include/scene/scene.h"
#include "../include/core/surface.h"

// NAMESPACE

// DEFINES

// STATIC MEMBER INITIALIZATION

/////////////////////////////////////////////////////////////////////////////////////////////

int main(int argc, char **argv)
{
	s_pCoreManager = Singleton<CoreManager>::init();

	coreM->initialize();
	sceneM->init();
	gpuPipelineM->init();

	bool isWindowOpen = true;
	float elapsedTimeMiliseconds = 0.0f;
	std::chrono::steady_clock::time_point rasterTime0;
	std::chrono::steady_clock::time_point rasterTime1;

	while (isWindowOpen)
	{
		if (coreM->getReachedFirstRaster() && !coreM->getIsPrepared())
		{
			continue;
		}

		MSG msg;
		while (PeekMessage(&msg, NULL, 0, 0, PM_REMOVE))
		{
			rasterTime0 = std::chrono::high_resolution_clock::now();

			TranslateMessage(&msg);
			DispatchMessage(&msg);

			if (coreM->getEndApplicationMessage())
			{
				coreM->deInitialize();
				PostQuitMessage(0);
				return false;
			}

			elapsedTimeMiliseconds = clamp(elapsedTimeMiliseconds, 0.0f, 10000.0f);

			sceneM->setDeltaTime(elapsedTimeMiliseconds);
			inputM->updateInput(msg);
			inputM->updateKeyboard();
			sceneM->update();
			gpuPipelineM->update();
			coreM->prepare();
			RedrawWindow(coreM->getWindowPlatformHandle(), NULL, NULL, RDW_INTERNALPAINT);
			coreM->render();
			coreM->postRender();

			rasterTime1 = std::chrono::high_resolution_clock::now();
			elapsedTimeMiliseconds = float(std::chrono::duration_cast<std::chrono::milliseconds>(rasterTime1 - rasterTime0).count());
		}
	}
	coreM->deInitialize();
}

/////////////////////////////////////////////////////////////////////////////////////////////
