using UnityEngine;
using System.IO;
using System.Runtime.Serialization.Formatters.Binary;

public static class SaveProgress {

    const string download = "C:/Users/m020115k/Downloads";

    public static void SaveData (GameManager manager)
    {
        BinaryFormatter formatter = new BinaryFormatter();
        string path = download + "/gamedata.pet";
        FileStream stream = new FileStream(path, FileMode.Create);

        GameData data = new GameData(manager);

        formatter.Serialize(stream, data);
        stream.Close();
    }

    public static GameData LoadData()
    {
        string path = download + "/gamedata.pet";
        if (File.Exists(path))
        {
            BinaryFormatter formatter = new BinaryFormatter();
            FileStream stream = new FileStream(path, FileMode.Open);

            GameData data = formatter.Deserialize(stream) as GameData;
            stream.Close();

            return data;

        }
        else {
            Debug.LogError("Save file not found on system! See: " + path);
            return null;
        }
    }
}
