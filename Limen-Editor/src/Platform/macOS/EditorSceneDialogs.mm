#include "UI/EditorSceneDialogs.h"

#import <AppKit/AppKit.h>
#import <UniformTypeIdentifiers/UniformTypeIdentifiers.h>

namespace LimenEditor
{
    namespace
    {
        EditorSceneDialogResult RunScenePanel(
            NSSavePanel *panel,
            const std::filesystem::path &initialDirectory,
            std::filesystem::path &outPath)
        {
            UTType *sceneType = [UTType typeWithFilenameExtension:@"lscene"];
            if (!sceneType)
                return EditorSceneDialogResult::Unavailable;

            panel.allowedContentTypes = @[sceneType];
            panel.allowsOtherFileTypes = NO;
            if (!initialDirectory.empty())
            {
                panel.directoryURL = [NSURL fileURLWithFileSystemRepresentation:initialDirectory.c_str()
                                                                       isDirectory:YES
                                                                     relativeToURL:nil];
            }

            if ([panel runModal] != NSModalResponseOK)
                return EditorSceneDialogResult::Cancelled;

            // NSURL 的文件系统表示可以直接转给使用原生路径编码的 std::filesystem。
            const char *path = panel.URL.fileSystemRepresentation;
            if (!path || !*path)
                return EditorSceneDialogResult::Unavailable;

            outPath = std::filesystem::path(path);
            return EditorSceneDialogResult::Accepted;
        }
    }

    EditorSceneDialogResult ChooseSceneToOpen(
        const std::filesystem::path &initialDirectory,
        std::filesystem::path &outPath)
    {
        @autoreleasepool
        {
            NSOpenPanel *panel = [NSOpenPanel openPanel];
            panel.title = @"Open Scene";
            panel.canChooseFiles = YES;
            panel.canChooseDirectories = NO;
            panel.allowsMultipleSelection = NO;
            return RunScenePanel(panel, initialDirectory, outPath);
        }
    }

    EditorSceneDialogResult ChooseSceneToSave(
        const std::filesystem::path &initialDirectory,
        const std::filesystem::path &suggestedFileName,
        std::filesystem::path &outPath)
    {
        @autoreleasepool
        {
            NSSavePanel *panel = [NSSavePanel savePanel];
            panel.title = @"Save Scene As";
            panel.canCreateDirectories = YES;
            panel.canSelectHiddenExtension = NO;

            const auto fileName = suggestedFileName.filename();
            if (!fileName.empty())
            {
                panel.nameFieldStringValue = [NSFileManager.defaultManager
                    stringWithFileSystemRepresentation:fileName.c_str() length:fileName.native().size()];
            }
            return RunScenePanel(panel, initialDirectory, outPath);
        }
    }

    EditorUnsavedSceneAction ConfirmUnsavedScene()
    {
        @autoreleasepool
        {
            NSAlert *alert = [[NSAlert alloc] init];
            alert.alertStyle = NSAlertStyleWarning;
            alert.messageText = @"Save changes to the current scene?";
            alert.informativeText = @"Unsaved changes will be lost if you discard them.";
            [alert addButtonWithTitle:@"Save"];
            [alert addButtonWithTitle:@"Discard Changes"];
            [alert addButtonWithTitle:@"Cancel"];

            switch ([alert runModal])
            {
                case NSAlertFirstButtonReturn: return EditorUnsavedSceneAction::Save;
                case NSAlertSecondButtonReturn: return EditorUnsavedSceneAction::Discard;
                default: return EditorUnsavedSceneAction::Cancel;
            }
        }
    }
}
