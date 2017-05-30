/*==============================================================================
Copyright (c) 2016 PTC Inc. All Rights Reserved.
Vuforia is a trademark of PTC Inc., registered in the United States and other 
countries.  
==============================================================================*/
using UnityEngine;
using UnityEditor;

namespace Vuforia.EditorClasses
{
    [InitializeOnLoad]
    public static class VideoPlaybackImport
    {
        static VideoPlaybackImport() 
        {
            EditorApplication.update += UpdateVuforiaMediaPluginSettings;
        }

        static void UpdateVuforiaMediaPluginSettings()
        {
            // Unregister callback (executed only once)
            EditorApplication.update -= UpdateVuforiaMediaPluginSettings;

            PluginImporter[] importers = PluginImporter.GetAllImporters();
            foreach (var imp in importers)
            {
                string pluginPath = imp.assetPath;
                if (pluginPath.EndsWith("VuforiaMediaSource") && imp.GetCompatibleWithAnyPlatform())
                {
                    Debug.Log("Disabling 'Any Platform' for plugin: " + pluginPath);
                    imp.SetCompatibleWithAnyPlatform(false);
                }
            }
        }    
    }
}
