using UnityEngine;
using System.Collections;

public class TextAnimation : MonoBehaviour {
    private GameObject mTextView;
    public float speed = 1.0F;

    // Use this for initialization
    void Start () {
        mTextView = GameObject.Find("ClickMeText");
    }
	
	// Update is called once per frame
	void Update () {
        //mTextView.transform.RotateAround(Vector3.right * speed * Time.deltaTime, speed);
        //mTextView.transform.Translate(Vector3.left * speed * Time.deltaTime, Space.World);
        mTextView.transform.Rotate(Vector3.left * speed * Time.deltaTime, Space.World);
        //mTextView.transform.RotateAround();
    }
}
