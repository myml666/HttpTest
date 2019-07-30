package com.itfitness.httptest;

import android.support.v7.app.AppCompatActivity;
import android.os.Bundle;
import android.view.View;
import android.widget.Button;
import android.widget.EditText;
import android.widget.TextView;

public class MainActivity extends AppCompatActivity {
    private final String REQUESTURL = "http://192.168.0.137:8080/Login/login";
    private Button mBtLogin;
    private EditText mEtUsername,mEtPassword;
    // Used to load the 'native-lib' library on application startup.
    static {
        System.loadLibrary("native-lib");
    }

    @Override
    protected void onCreate(Bundle savedInstanceState) {
        super.onCreate(savedInstanceState);
        setContentView(R.layout.activity_main);

        // Example of a call to a native method
        final TextView tv = findViewById(R.id.sample_text);
        mBtLogin = findViewById(R.id.bt_login);
        mEtPassword = findViewById(R.id.et_password);
        mEtUsername = findViewById(R.id.et_username);
        mBtLogin.setOnClickListener(new View.OnClickListener() {
            @Override
            public void onClick(View v) {
                tv.setText(stringFromJNI(REQUESTURL,"username="+mEtUsername.getText().toString().trim()+"&password="+mEtPassword.getText().toString().trim()));
            }
        });
    }

    /**
     * A native method that is implemented by the 'native-lib' native library,
     * which is packaged with this application.
     */
    public native String stringFromJNI(String requestUrl,String params);
}
