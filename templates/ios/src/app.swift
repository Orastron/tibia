/*
 * Tibia
 *
 * Copyright (C) 2023, 2024 Orastron Srl unipersonale
 *
 * Tibia is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, version 3 of the License.
 *
 * Tibia is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with Tibia.  If not, see <http://www.gnu.org/licenses/>.
 *
 * File author: Stefano D'Angelo, Paolo Marrone
 */

import SwiftUI
@preconcurrency import WebKit
import AVFoundation

struct WebView: UIViewRepresentable {
	class Coordinator: NSObject, WKScriptMessageHandlerWithReply, WKNavigationDelegate {
		func userContentController(_ userContentController: WKUserContentController, didReceive message: WKScriptMessage, replyHandler: @escaping (Any?, String?) -> Void) {
			guard let body = message.body as? [String : Any] else { return }
			guard let name = body["name"] as? String else { return }
			switch (name) {
			case "needAudioPermission":
				replyHandler(AVCaptureDevice.authorizationStatus(for: .audio) != .authorized, nil)
				break;
			case "requestAudioPermission":
				AVAudioSession.sharedInstance().requestRecordPermission { granted in }
				replyHandler(nil, nil)
				break;
			case "audioStart":
				replyHandler(audioStart() != 0, nil)
				break;
			case "audioStop":
				audioStop()
				replyHandler(nil, nil)
				break;
			case "setParameter":
				guard let index = body["index"] as? Int32 else { return }
				guard let value = body["value"] as? Double else { return }
				setParameter(index, Float(value))
				replyHandler(nil, nil)
				break;
			case "getParameter":
				guard let index = body["index"] as? Int32 else { return }
				let v = getParameter(index)
				replyHandler(v, nil)
				break;
			default:
				break;
			}
		}
		
		func webView(_ webView: WKWebView, decidePolicyFor navigationAction: WKNavigationAction, decisionHandler: @escaping (WKNavigationActionPolicy) -> Void) {
			if let url = navigationAction.request.url, navigationAction.navigationType == .linkActivated {
				UIApplication.shared.open(url)
				decisionHandler(.cancel)
			} else {
				decisionHandler(.allow)
			}
		}
	}

	func makeCoordinator() -> Coordinator {
		return Coordinator()
	}

	let url: URL

	func makeUIView(context: Context) -> WKWebView {
		let configuration = WKWebViewConfiguration()
		configuration.userContentController.addScriptMessageHandler(Coordinator(), contentWorld: .page, name: "messageHandler")
		configuration.setValue(true, forKey: "allowUniversalAccessFromFileURLs")
		let webView = WKWebView(frame: .zero, configuration: configuration)
		webView.navigationDelegate = context.coordinator
		//webView.isInspectable = true
		return webView
	}

	func updateUIView(_ webView: WKWebView, context: Context) {
		let request = URLRequest(url: url)
		webView.load(request)
	}
}

struct ContentView: View {
	var body: some View {
		let url = Bundle.main.url(forResource: "index", withExtension: "html", subdirectory: "res")
		WebView(url: url!).ignoresSafeArea().preferredColorScheme(.dark)
	}
}

struct ContentView_Previews: PreviewProvider {
	static var previews: some View {
		ContentView()
	}
}

@main
struct templateApp: App {
	var body: some Scene {
		WindowGroup {
			ContentView()
				.onReceive(NotificationCenter.default.publisher(for: UIApplication.didEnterBackgroundNotification)) { _ in
					audioPause()
				}
				.onReceive(NotificationCenter.default.publisher(for: UIApplication.willEnterForegroundNotification)) { _ in
					audioResume()
				}
		}
	}
}
