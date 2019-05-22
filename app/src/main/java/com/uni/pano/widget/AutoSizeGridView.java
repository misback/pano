package com.uni.pano.widget;

import android.content.Context;
import android.util.AttributeSet;
import android.widget.GridView;

public class AutoSizeGridView extends GridView {

    public AutoSizeGridView(Context context) {
        super(context);
        // TODO Auto-generated constructor stub
    }

    public AutoSizeGridView(Context context, AttributeSet attrs) {
        super(context, attrs);
        // TODO Auto-generated constructor stub
    }

    public AutoSizeGridView(Context context, AttributeSet attrs, int defStyle) {
        super(context, attrs, defStyle);
        // TODO Auto-generated constructor stub
    }

    @Override
    public void onMeasure(int widthMeasureSpec, int heightMeasureSpec) {

        int expandSpec = MeasureSpec.makeMeasureSpec(
                Integer.MAX_VALUE >> 2, MeasureSpec.AT_MOST);
        super.onMeasure(widthMeasureSpec, expandSpec);
    }

}
