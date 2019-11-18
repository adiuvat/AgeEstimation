EN|[CN](README_cn.md)

Developers can deploy the application on the Atlas 200 DK to collect camera data in real time and predict facial information in the video.

## Prerequisites<a name="en-us_topic_0167089636_section412314183119"></a>

Before using an open source application, ensure that:

-   MindSpore Studio has been installed. For details, see  [MindSpore Studio Installation Guide](https://www.huawei.com/minisite/ascend/en/filedetail_1.html).
-   The Atlas 200 DK developer board has been connected to MindSpore Studio, the cross compiler has been installed, the SD card has been prepared, and basic information has been configured. For details, see  [Atlas 200 DK User Guide](https://www.huawei.com/minisite/ascend/en/filedetail_2.html).

## Software Preparation<a name="en-us_topic_0167089636_section177411912193214"></a>

Before running the application, obtain the source code package and configure the environment as follows.

1.  Obtain the source code package.

    Download all the code in the sample-facedetection repository at  [https://github.com/Ascend/sample-facedetection](https://github.com/Ascend/sample-facedetection)  to any directory on Ubuntu Server where MindSpore Studio is located as the MindSpore Studio installation user, for example,  _/home/ascend/sample-facedetection/_.

2.  Log in to Ubuntu Server where MindSpore Studio is located as the MindSpore Studio installation user and set the environment variable  **DDK\_HOME**.

    **vim \~/.bashrc**

    Run the following commands to add the environment variables  **DDK\_HOME**  and  **LD\_LIBRARY\_PATH**  to the last line:

    **export DDK\_HOME=/home/XXX/tools/che/ddk/ddk**

    **export LD\_LIBRARY\_PATH=$DDK\_HOME/uihost/lib**

    >![](doc/source/img/icon-note.gif) **NOTE:**   
    >-   **XXX**  indicates the MindSpore Studio installation user, and  **/home/XXX/tools**  indicates the default installation path of the DDK.  
    >-   If the environment variables have been added, skip this step.  

    Enter  **:wq!**  to save and exit.

    Run the following command for the environment variable to take effect:

    **source \~/.bashrc**


## Deployment<a name="en-us_topic_0167089636_section15718149133616"></a>

1.  Access the root directory where the face detection application code is located as the MindSpore Studio installation user, for example,  _**/home/ascend/sample-facedetection**_.
2.  Run the deployment script to prepare the project environment, including compiling and deploying the ascenddk public library, downloading the network model, and configuring Presenter Server.

    **bash deploy.sh** _host\_ip_ _model\_mode_

    -   _host\_ip_: For the Atlas 200 DK developer board, this parameter indicates the IP address of the developer board.
    -   _model\_mode_  indicates the deployment mode of the model file. The default setting is  **internet**.
        -   **local**: If the Ubuntu system where MindSpore Studio is located is not connected to the network, use the local mode. In this case, download the network model file and the dependent common code library to the  **sample-facedetection/script**  directory by referring to the  [Downloading Network Model and Dependency Code Library](#en-us_topic_0167089636_section193081336153717).
        -   **internet**: Indicates the online deployment mode. If the Ubuntu system where MindSpore Studio is located is connected to the network, use the Internet mode. In this case, download the model file and  dependency code library online.


    Example command:

    **bash deploy.sh 192.168.1.2 internet**

    When the message  **Please choose one to show the presenter in browser\(default: 127.0.0.1\):**  is displayed, enter the IP address used for accessing the Presenter Server service in the browser. Generally, the IP address is the IP address for accessing the MindSpore Studio service.

    Select the IP address used by the browser to access the Presenter Server service in  **Current environment valid ip list**, as shown in  [Figure 1](#en-us_topic_0167089636_fig184321447181017).

    **Figure  1**  Project deployment<a name="en-us_topic_0167089636_fig184321447181017"></a>  
    ![](doc/source/img/project-deployment.png "project-deployment")

3.  <a name="en-us_topic_0167089636_li499911453439"></a>Start Presenter Server.

    Run the following command to start the Presenter Server program of the face detection application in the background:

    **python3 presenterserver/presenter\_server.py --app face\_detection &**

    >![](doc/source/img/icon-note.gif) **NOTE:**   
    >**presenter\_server.py**  is located in the  **presenterserver**  in the current directory. You can run the  **python3 presenter\_server.py -h**  or  **python3 presenter\_server.py --help**  command in this directory to view the usage method of  **presenter\_server.py**.  

    [Figure 2](#en-us_topic_0167089636_fig69531305324)  shows that the presenter\_server service is started successfully.

    **Figure  2**  Starting the Presenter Server process<a name="en-us_topic_0167089636_fig69531305324"></a>  
    ![](doc/source/img/starting-the-presenter-server-process.png "starting-the-presenter-server-process")

    Use the URL shown in the preceding figure to log in to Presenter Server \( only the Chrome browser is supporte \). The IP address is that entered in  [Figure 3](#en-us_topic_0167089636_fig64391558352)  and the default port number is  **7007**. The following figure indicates that Presenter Server is started successfully.

    **Figure  3**  Home page<a name="en-us_topic_0167089636_fig64391558352"></a>  
    ![](doc/source/img/home-page.png "home-page")


## Running<a name="en-us_topic_0167089636_section10271726154420"></a>

1.  Run the face detection application.

    Run the following command in the  **sample-facedetection**  directory to start the face detection application:

    **bash run\_facedetectionapp.sh** _host\_ip_ _presenter\_view\_app\_name camera\_channel\_name_  &

    -   _host\_ip_: For the Atlas 200 DK developer board, this parameter indicates the IP address of the developer board.
    -   _presenter\_view\_app\_name_: Indicates  **View Name**  displayed on the Presenter Server page, which is user-defined.
    -   _camera\_channel\_name_: Indicates the channel to which a camera belongs. The value can be  **Channel-1**  or  **Channel-2**. For details, see  **Common Operations > View the Channel to Which a Camera Belongs** of [Atlas 200 DK User Guide](https://www.huawei.com/minisite/ascend/en/filedetail_2.html).

    Example command:

    **bash run\_facedetectionapp.sh 192.168.1.2 video Channel-1 &**

2.  Use the URL that is displayed when you start the Presenter Server service to log in to the Presenter Server website. For details, see  [3](#en-us_topic_0167089636_li499911453439).

    Wait for Presenter Agent to transmit data to the server. Click  **Refresh**. When there is data, the icon in the  **Status**  column for the corresponding channel changes to green, as shown in  [Figure 4](#en-us_topic_0167089636_fig113691556202312).

    **Figure  4**  Presenter Server page<a name="en-us_topic_0167089636_fig113691556202312"></a>  
    ![](doc/source/img/presenter-server-page.png "presenter-server-page")

    >![](doc/source/img/icon-note.gif) **NOTE:**   
    >-   The Presenter Server of the face detection application supports a maximum of 10 channels at the same time , each  presenter\_view\_app\_name  parameter corresponds to a channel.  
    >-   Due to hardware limitations, the maximum frame rate supported by each channel is 20fps,  a lower frame rate is automatically used when the network bandwidth is low.  

3.  Click  **image**  or  **video**  in the  **View Name**  column and view the result. The confidence of the detected face is marked.

## Follow-up Operations<a name="en-us_topic_0167089636_section1092612277429"></a>

-   **Stopping the Face Detection Application**

    The face detection application is running continually after being executed. To stop it, perform the following operation:

    Run the following command in the  _**/home/ascend/sample-facedetection**_  directory as the MindSpore Studio installation user:

    **bash stop\_facedetectionapp.sh** _host\_ip_

    _host\_ip_: For the Atlas 200 DK developer board, this parameter indicates the IP address of the developer board.For the Atlas 300 PCIe card, this parameter indicates the IP address of the PCIe card host.

    Example command:

    **bash stop\_facedetectionapp.sh 192.168.1.2**

-   **Stopping the Presenter Server Service**

    The Presenter Server service is always in the running state after being started. To stop the Presenter Server service of the face detection application, perform the following operations:

    Run the following command to check the process of the Presenter Server service corresponding to the face detection application as the MindSpore Studio installation user:

    **ps -ef | grep presenter | grep face\_detection**

    ```
    ascend@ascend-HP-ProDesk-600-G4-PCI-MT:~/sample-facedetection$ ps -ef | grep presenter | grep face_detection
    ascend    7701  1615  0 14:21 pts/8    00:00:00 python3 presenterserver/presenter_server.py --app face_detection
    ```

    In the preceding information,  _7701_  indicates the process ID of the Presenter Server service corresponding to the face detection application.

    To stop the service, run the following command:

    **kill -9** _7701_


## Downloading Network Model and Dependency Code Library<a name="en-us_topic_0167089636_section193081336153717"></a>

-   Downloading network model

    The models used in the application are converted models that adapt to the Ascend 310 chipset. For details about how to download this kind of models and the original network models of face detection application, see  [Table 1](#en-us_topic_0167089636_table0531392153). If you have a better model solution, you are welcome to share it at  [https://github.com/Ascend/models](https://github.com/Ascend/models).

    Download the network models files (.om files) to the **sample-facedetection/script** directory.

    **Table  1**  Models used in face detection applications

    <a name="en-us_topic_0167089636_table0531392153"></a>
    <table><thead align="left"><tr id="en-us_topic_0167089636_row1154103991514"><th class="cellrowborder" valign="top" width="15.841584158415841%" id="mcps1.2.5.1.1"><p id="en-us_topic_0167089636_p195418397155"><a name="en-us_topic_0167089636_p195418397155"></a><a name="en-us_topic_0167089636_p195418397155"></a>Model Name</p>
    </th>
    <th class="cellrowborder" valign="top" width="21.782178217821784%" id="mcps1.2.5.1.2"><p id="en-us_topic_0167089636_p1054539151519"><a name="en-us_topic_0167089636_p1054539151519"></a><a name="en-us_topic_0167089636_p1054539151519"></a>Description</p>
    </th>
    <th class="cellrowborder" valign="top" width="28.71287128712871%" id="mcps1.2.5.1.3"><p id="en-us_topic_0167089636_p387083117108"><a name="en-us_topic_0167089636_p387083117108"></a><a name="en-us_topic_0167089636_p387083117108"></a>Model Download Path</p>
    </th>
    <th class="cellrowborder" valign="top" width="33.663366336633665%" id="mcps1.2.5.1.4"><p id="en-us_topic_0167089636_p35412397154"><a name="en-us_topic_0167089636_p35412397154"></a><a name="en-us_topic_0167089636_p35412397154"></a>Original Network Download Address</p>
    </th>
    </tr>
    </thead>
    <tbody><tr id="en-us_topic_0167089636_row65414393159"><td class="cellrowborder" valign="top" width="15.841584158415841%" headers="mcps1.2.5.1.1 "><p id="en-us_topic_0167089636_p17544398153"><a name="en-us_topic_0167089636_p17544398153"></a><a name="en-us_topic_0167089636_p17544398153"></a>Network model for face detection</p>
    <p id="en-us_topic_0167089636_p84114461512"><a name="en-us_topic_0167089636_p84114461512"></a><a name="en-us_topic_0167089636_p84114461512"></a>(<strong id="en-us_topic_0167089636_b41111030191911"><a name="en-us_topic_0167089636_b41111030191911"></a><a name="en-us_topic_0167089636_b41111030191911"></a>face_detection.om</strong>)</p>
    </td>
    <td class="cellrowborder" valign="top" width="21.782178217821784%" headers="mcps1.2.5.1.2 "><p id="en-us_topic_0167089636_p1372429181516"><a name="en-us_topic_0167089636_p1372429181516"></a><a name="en-us_topic_0167089636_p1372429181516"></a>It is a network model converted from ResNet10-SSD300 model based on Caffe.</p>
    </td>
    <td class="cellrowborder" valign="top" width="28.71287128712871%" headers="mcps1.2.5.1.3 "><p id="en-us_topic_0167089636_p1569513572242"><a name="en-us_topic_0167089636_p1569513572242"></a><a name="en-us_topic_0167089636_p1569513572242"></a>Download the model from the <strong id="en-us_topic_0167089636_b028612482311"><a name="en-us_topic_0167089636_b028612482311"></a><a name="en-us_topic_0167089636_b028612482311"></a>computer_vision/object_detect/face_detection</strong> directory in the <a href="https://github.com/Ascend/models/" target="_blank" rel="noopener noreferrer">https://github.com/Ascend/models/</a> repository.</p>
    <p id="en-us_topic_0167089636_p1787118315101"><a name="en-us_topic_0167089636_p1787118315101"></a><a name="en-us_topic_0167089636_p1787118315101"></a>For the version description, see the <strong id="en-us_topic_0167089636_b1012219832511"><a name="en-us_topic_0167089636_b1012219832511"></a><a name="en-us_topic_0167089636_b1012219832511"></a>README.md</strong> file in the current directory.</p>
    </td>
    <td class="cellrowborder" valign="top" width="33.663366336633665%" headers="mcps1.2.5.1.4 "><p id="en-us_topic_0167089636_p1785381617217"><a name="en-us_topic_0167089636_p1785381617217"></a><a name="en-us_topic_0167089636_p1785381617217"></a>For details, see the <strong id="en-us_topic_0167089636_b1423252411265"><a name="en-us_topic_0167089636_b1423252411265"></a><a name="en-us_topic_0167089636_b1423252411265"></a>README.md</strong> file of the <strong id="en-us_topic_0167089636_b688544332614"><a name="en-us_topic_0167089636_b688544332614"></a><a name="en-us_topic_0167089636_b688544332614"></a>computer_vision/object_detect/face_detection</strong> directory in the <a href="https://github.com/Ascend/models/" target="_blank" rel="noopener noreferrer">https://github.com/Ascend/models/</a> repository.</p>
    <p id="en-us_topic_0167089636_p1314312124919"><a name="en-us_topic_0167089636_p1314312124919"></a><a name="en-us_topic_0167089636_p1314312124919"></a><strong id="en-us_topic_0167089636_b1365251225519"><a name="en-us_topic_0167089636_b1365251225519"></a><a name="en-us_topic_0167089636_b1365251225519"></a>Precautions during model conversion:</strong></p>
    <p id="en-us_topic_0167089636_p53116302463"><a name="en-us_topic_0167089636_p53116302463"></a><a name="en-us_topic_0167089636_p53116302463"></a>During the conversion, a message is displayed indicating that the conversion fails. You only need to select <strong id="en-us_topic_0167089636_b55978299556"><a name="en-us_topic_0167089636_b55978299556"></a><a name="en-us_topic_0167089636_b55978299556"></a>SSDDetectionOutput </strong>from the drop-down list box for the last layer and click <strong id="en-us_topic_0167089636_b15597182918551"><a name="en-us_topic_0167089636_b15597182918551"></a><a name="en-us_topic_0167089636_b15597182918551"></a>Retry</strong>.</p>
    <p id="en-us_topic_0167089636_p109405475158"><a name="en-us_topic_0167089636_p109405475158"></a><a name="en-us_topic_0167089636_p109405475158"></a><a name="en-us_topic_0167089636_image13957135893610"></a><a name="en-us_topic_0167089636_image13957135893610"></a><span><img id="en-us_topic_0167089636_image13957135893610" src="doc/source/img/en-us_image_0167757543.png"></span></p>
    <p id="en-us_topic_0167089636_p179225194910"><a name="en-us_topic_0167089636_p179225194910"></a><a name="en-us_topic_0167089636_p179225194910"></a></p>
    </td>
    </tr>
    </tbody>
    </table>

-   Download the dependent software libraries
    
    Download the dependent software libraries to the **sample-facedetection/script** directory.

    **Table  2**  Download the dependent software library

    <a name="en-us_topic_0167089636_table141761431143110"></a>
    <table><thead align="left"><tr id="en-us_topic_0167089636_row18177103183119"><th class="cellrowborder" valign="top" width="33.33333333333333%" id="mcps1.2.4.1.1"><p id="en-us_topic_0167089636_p8177331103112"><a name="en-us_topic_0167089636_p8177331103112"></a><a name="en-us_topic_0167089636_p8177331103112"></a>Module Name</p>
    </th>
    <th class="cellrowborder" valign="top" width="33.33333333333333%" id="mcps1.2.4.1.2"><p id="en-us_topic_0167089636_p1317753119313"><a name="en-us_topic_0167089636_p1317753119313"></a><a name="en-us_topic_0167089636_p1317753119313"></a>Module Description</p>
    </th>
    <th class="cellrowborder" valign="top" width="33.33333333333333%" id="mcps1.2.4.1.3"><p id="en-us_topic_0167089636_p1417713111311"><a name="en-us_topic_0167089636_p1417713111311"></a><a name="en-us_topic_0167089636_p1417713111311"></a>Download Address</p>
    </th>
    </tr>
    </thead>
    <tbody><tr id="en-us_topic_0167089636_row19177133163116"><td class="cellrowborder" valign="top" width="33.33333333333333%" headers="mcps1.2.4.1.1 "><p id="en-us_topic_0167089636_p2017743119318"><a name="en-us_topic_0167089636_p2017743119318"></a><a name="en-us_topic_0167089636_p2017743119318"></a>EZDVPP</p>
    </td>
    <td class="cellrowborder" valign="top" width="33.33333333333333%" headers="mcps1.2.4.1.2 "><p id="en-us_topic_0167089636_p52110611584"><a name="en-us_topic_0167089636_p52110611584"></a><a name="en-us_topic_0167089636_p52110611584"></a>Encapsulates the dvpp interface and provides image and video processing capabilities, such as color gamut conversion and image / video conversion</p>
    </td>
    <td class="cellrowborder" valign="top" width="33.33333333333333%" headers="mcps1.2.4.1.3 "><p id="en-us_topic_0167089636_p31774315318"><a name="en-us_topic_0167089636_p31774315318"></a><a name="en-us_topic_0167089636_p31774315318"></a><a href="https://github.com/Ascend/sdk-ezdvpp" target="_blank" rel="noopener noreferrer">https://github.com/Ascend/sdk-ezdvpp</a></p>
    <p id="en-us_topic_0167089636_p1634523015710"><a name="en-us_topic_0167089636_p1634523015710"></a><a name="en-us_topic_0167089636_p1634523015710"></a>After the download, keep the folder name <span class="filepath" id="en-us_topic_0167089636_filepath1324864613582"><a name="en-us_topic_0167089636_filepath1324864613582"></a><a name="en-us_topic_0167089636_filepath1324864613582"></a><b>ezdvpp</b></span>。</p>
    </td>
    </tr>
    <tr id="en-us_topic_0167089636_row101773315313"><td class="cellrowborder" valign="top" width="33.33333333333333%" headers="mcps1.2.4.1.1 "><p id="en-us_topic_0167089636_p217773153110"><a name="en-us_topic_0167089636_p217773153110"></a><a name="en-us_topic_0167089636_p217773153110"></a>Presenter Agent</p>
    </td>
    <td class="cellrowborder" valign="top" width="33.33333333333333%" headers="mcps1.2.4.1.2 "><p id="en-us_topic_0167089636_p19431399359"><a name="en-us_topic_0167089636_p19431399359"></a><a name="en-us_topic_0167089636_p19431399359"></a><span>API for interacting with the Presenter Server</span>.</p>
    </td>
    <td class="cellrowborder" valign="top" width="33.33333333333333%" headers="mcps1.2.4.1.3 "><p id="en-us_topic_0167089636_p16684144715560"><a name="en-us_topic_0167089636_p16684144715560"></a><a name="en-us_topic_0167089636_p16684144715560"></a><a href="https://github.com/Ascend/sdk-presenter/tree/master/presenteragent" target="_blank" rel="noopener noreferrer">https://github.com/Ascend/sdk-presenter/tree/master/presenteragent</a></p>
    <p id="en-us_topic_0167089636_p82315442578"><a name="en-us_topic_0167089636_p82315442578"></a><a name="en-us_topic_0167089636_p82315442578"></a>After the download, keep the folder name <span class="filepath" id="en-us_topic_0167089636_filepath19800155745817"><a name="en-us_topic_0167089636_filepath19800155745817"></a><a name="en-us_topic_0167089636_filepath19800155745817"></a><b>presenteragent</b></span>.</p>
    </td>
    </tr>
    <tr id="en-us_topic_0167333650_row101773315313"><td class="cellrowborder" valign="top" width="33.33333333333333%" headers="mcps1.2.4.1.1 "><p>tornado (5.1.0)</p><p>protobuf (3.5.1)</p><p>numpy (1.14.2)</P>
</td>
<td class="cellrowborder" valign="top" width="33.33333333333333%" headers="mcps1.2.4.1.2 "><p id="en-us_topic_0167333650_p19431399359"><a name="en-us_topic_0167333650_p19431399359"></a><a name="en-us_topic_0167333650_p19431399359"></a><span>Python libraries that Presenter Server depends on.</span>.</p>
</td>
<td class="cellrowborder" valign="top" width="33.33333333333333%" headers="mcps1.2.4.1.3 "><p id="en-us_topic_0167333650_p16684144715560">Search for related sources and install them.</p>
</td>
</tr>
    </tbody>
    </table>


