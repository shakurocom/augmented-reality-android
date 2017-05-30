using UnityEngine;
using System.Collections;
using Vuforia;
using System;

public class vbScript : MonoBehaviour, IVirtualButtonEventHandler {

    private GameObject vbButtonObject;
    private GameObject zombie;

    public float speed = 50.0f;

    /*void IVirtualButtonEventHandler.OnButtonPressed(VirtualButtonAbstractBehaviour vb)
    {
        throw new NotImplementedException();
    }

    void IVirtualButtonEventHandler.OnButtonReleased(VirtualButtonAbstractBehaviour vb)
    {
        throw new NotImplementedException();
    }*/

    // Use this for initialization
    void Start () {
        vbButtonObject = GameObject.Find("actionButton");
        zombie = GameObject.Find("zombie");
        vbButtonObject.GetComponent<VirtualButtonBehaviour>().RegisterEventHandler(this);
	}

    void Update()
    {
        // zombie.transform.Translate(Vector3.left * speed * Time.deltaTime, Space.World);
        // zombie.GetComponent<Animation>().Play();
    }

    public void OnButtonPressed(VirtualButtonAbstractBehaviour vb)
    {
        Debug.Log("button down!");
        zombie.GetComponent<Animation>().Play();
    }

    public void OnButtonReleased(VirtualButtonAbstractBehaviour vb)
    {
        zombie.GetComponent<Animation>().Stop();
    }
}
