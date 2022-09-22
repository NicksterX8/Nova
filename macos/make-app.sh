readonly MAC_APP_PATH=build/macos/Faketorio.app
mkdir -p $MAC_APP_PATH
mkdir -p $MAC_APP_PATH/Contents
cp macos/info.plist $MAC_APP_PATH/Contents/info.plist
mkdir -p $MAC_APP_PATH/Contents/MacOS
cp build/faketorio $MAC_APP_PATH/Contents/MacOS/exe
cp -r build/resources $MAC_APP_PATH/Contents/
cp macos/favicon.icns $MAC_APP_PATH/Contents/Resources/favicon.icns
cp macos/PkgInfo $MAC_APP_PATH/Contents/Resources/PkgInfo