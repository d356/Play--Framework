#pragma once

#include "TrayIcon.h"
#include <functional>
#include <memory>
#include <map>
#include <boost/signals2.hpp>

namespace Framework
{
	namespace Win32
	{
		class CTrayIconServer : public CWindow
		{
		public:
			typedef std::function<void (CTrayIcon*, LPARAM)> IconEventHandlerType;

									CTrayIconServer();
			virtual					~CTrayIconServer();

			CTrayIcon*				Insert();

			void					RegisterHandler(const IconEventHandlerType&);

		protected:
			long					OnWndProc(unsigned int, WPARAM, LPARAM);

		private:
			typedef std::map<unsigned int, std::unique_ptr<CTrayIcon>> TrayIconMapType;
			typedef boost::signals2::signal<void (CTrayIcon*, LPARAM)> IconEventSignalType;

			TrayIconMapType			m_icons;
			IconEventSignalType		m_iconEventSignal;
			unsigned int			m_nextIconId = 1;
			unsigned int			m_taskBarCreatedMessage = 0;
		};
	}
}
