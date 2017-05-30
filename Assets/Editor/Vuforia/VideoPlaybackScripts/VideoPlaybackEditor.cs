/*==============================================================================
Copyright (c) 2015-2016 PTC Inc. All Rights Reserved.

Copyright (c) 2012-2015 Qualcomm Connected Experiences, Inc.
All Rights Reserved.

Vuforia is a trademark of PTC Inc., registered in the United States and other 
countries.  
==============================================================================*/

using UnityEditor;
using UnityEngine;

/// <summary>
/// This editor renders the inspector for the VideoPlaybackBehaviour
/// </summary>
[CustomEditor(typeof(VideoPlaybackBehaviour))]
public class VideoPlaybackEditor : Editor
{
    #region NESTED

    public const string REFERENCE_MATERIAL_PATH =
            "Assets/Vuforia Video Playback/Materials/VideoMaterial.mat";

    #endregion // NESTED



    #region UNITY_EDITOR_METHODS

    public override void OnInspectorGUI()
    {
        // record potential changes for this object
        Undo.RecordObject(target, "Inspector Change");

        // Get the video playback behaviour that is being edited
        VideoPlaybackBehaviour vpb = (VideoPlaybackBehaviour) target;

        // Draw the default inspector
        DrawDefaultInspector();

        // Add an inspector field for the keyframe texture
        vpb.KeyframeTexture = (Texture) EditorGUILayout.ObjectField(
            "Keyframe Texture", vpb.KeyframeTexture, typeof(Texture), false);

        // If the keyframe texture field changed, update the material
        if (GUI.changed)
        {
            UpdateMaterial(vpb);

            EditorUtility.SetDirty(vpb);
        }
    }

    #endregion // UNITY_EDITOR_METHODS



    #region PRIVATE_METHODS

    public static void UpdateMaterial(VideoPlaybackBehaviour vpb)
    {
        // Load the reference material
        Material referenceMaterial =
                (Material)AssetDatabase.LoadAssetAtPath(REFERENCE_MATERIAL_PATH,
                                                    typeof(Material));

        if (referenceMaterial == null)
        {
            Debug.LogError("Could not find reference material at " +
                           REFERENCE_MATERIAL_PATH +
                           " please reimport Unity package.");
            return;
        }

        if (vpb.KeyframeTexture == null)
        {
            // Reset to reference material if keyframe texture is null
            vpb.GetComponent<Renderer>().sharedMaterial = referenceMaterial;
        }
        else
        {
            // Create a new material that is based on the reference material and
            // uses the selected keyframe texture
            Material material = new Material(referenceMaterial);

            material.mainTexture = vpb.KeyframeTexture;
            material.name = vpb.KeyframeTexture.name + "Material";
            material.mainTextureScale = new Vector2(-1, -1);
            material.renderQueue = referenceMaterial.renderQueue + 1;

            vpb.GetComponent<Renderer>().sharedMaterial = material;
        }

        // Cleanup assets that have been created temporarily
        EditorUtility.UnloadUnusedAssetsImmediate();
    }

    #endregion // PRIVATE_METHODS
}
