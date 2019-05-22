package com.uni.common.util;

import android.util.SparseArray;
import android.view.View;

public class ViewHolderUtil {
    @SuppressWarnings("unchecked")
    public static <T extends View> T get(View view, int id) {
        SparseArray<View> viewHolder = (SparseArray<View>) view.getTag(view.getId());
        if (viewHolder == null) {
            viewHolder = new SparseArray<View>();
            view.setTag(view.getId(), viewHolder);
        }
        View childView = viewHolder.get(id);
        if (childView == null) {
            childView = view.findViewById(id);
            viewHolder.put(id, childView);
        }
        return (T) childView;
    }
}
