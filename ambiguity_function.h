
// ambiguity_function.h: главный файл заголовка для приложения PROJECT_NAME
//

#pragma once

#ifndef __AFXWIN_H__
	#error "включить pch.h до включения этого файла в PCH"
#endif

#include "resource.h"		// основные символы


// Cambiguity_functionApp:
// Сведения о реализации этого класса: ambiguity_function.cpp
//

class Cambiguity_functionApp : public CWinApp
{
public:
	Cambiguity_functionApp();

// Переопределение
public:
	virtual BOOL InitInstance();

// Реализация

	DECLARE_MESSAGE_MAP()
};

extern Cambiguity_functionApp theApp;
