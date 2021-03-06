/*
 * Copyright (C) 2014 The Android Open Source Project
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *      http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

package android.media.tv;

import android.annotation.SystemApi;
import android.graphics.Rect;
import android.net.Uri;
import android.os.Bundle;
import android.os.Handler;
import android.os.IBinder;
import android.os.Looper;
import android.os.Message;
import android.os.RemoteException;
import android.util.ArrayMap;
import android.util.Log;
import android.util.Pools.Pool;
import android.util.Pools.SimplePool;
import android.util.SparseArray;
import android.view.InputChannel;
import android.view.InputEvent;
import android.view.InputEventSender;
import android.view.KeyEvent;
import android.view.Surface;
import android.view.View;

import java.util.ArrayList;
import java.util.Iterator;
import java.util.LinkedList;
import java.util.List;
import java.util.Map;

/**
 * Central system API to the overall TV input framework (TIF) architecture, which arbitrates
 * interaction between applications and the selected TV inputs.
 */
public final class TvInputManager {
    private static final String TAG = "TvInputManager";

    static final int VIDEO_UNAVAILABLE_REASON_START = 0;
    static final int VIDEO_UNAVAILABLE_REASON_END = 3;

    /**
     * A generic reason. Video is not available due to an unspecified error.
     */
    public static final int VIDEO_UNAVAILABLE_REASON_UNKNOWN = VIDEO_UNAVAILABLE_REASON_START;
    /**
     * Video is not available because the TV input is in the middle of tuning to a new channel.
     */
    public static final int VIDEO_UNAVAILABLE_REASON_TUNING = 1;
    /**
     * Video is not available due to the weak TV signal.
     */
    public static final int VIDEO_UNAVAILABLE_REASON_WEAK_SIGNAL = 2;
    /**
     * Video is not available because the TV input stopped the playback temporarily to buffer more
     * data.
     */
    public static final int VIDEO_UNAVAILABLE_REASON_BUFFERING = VIDEO_UNAVAILABLE_REASON_END;

    /**
     * The TV input is in unknown state.
     * <p>
     * State for denoting unknown TV input state. The typical use case is when a requested TV
     * input is removed from the device or it is not registered. Used in
     * {@code ITvInputManager.getTvInputState()}.
     * </p>
     * @hide
     */
    public static final int INPUT_STATE_UNKNOWN = -1;

    /**
     * The TV input is connected.
     * <p>
     * State for {@link #getInputState} and {@link
     * TvInputManager.TvInputCallback#onInputStateChanged}.
     * </p>
     */
    public static final int INPUT_STATE_CONNECTED = 0;
    /**
     * The TV input is connected but in standby mode. It would take a while until it becomes
     * fully ready.
     * <p>
     * State for {@link #getInputState} and {@link
     * TvInputManager.TvInputCallback#onInputStateChanged}.
     * </p>
     */
    public static final int INPUT_STATE_CONNECTED_STANDBY = 1;
    /**
     * The TV input is disconnected.
     * <p>
     * State for {@link #getInputState} and {@link
     * TvInputManager.TvInputCallback#onInputStateChanged}.
     * </p>
     */
    public static final int INPUT_STATE_DISCONNECTED = 2;

    /**
     * Broadcast intent action when the user blocked content ratings change. For use with the
     * {@link #isRatingBlocked}.
     */
    public static final String ACTION_BLOCKED_RATINGS_CHANGED =
            "android.media.tv.action.BLOCKED_RATINGS_CHANGED";

    /**
     * Broadcast intent action when the parental controls enabled state changes. For use with the
     * {@link #isParentalControlsEnabled}.
     */
    public static final String ACTION_PARENTAL_CONTROLS_ENABLED_CHANGED =
            "android.media.tv.action.PARENTAL_CONTROLS_ENABLED_CHANGED";

    /**
     * Broadcast intent action used to query available content rating systems.
     * <p>
     * The TV input manager service locates available content rating systems by querying broadcast
     * receivers that are registered for this action. An application can offer additional content
     * rating systems to the user by declaring a suitable broadcast receiver in its manifest.
     * </p><p>
     * Here is an example broadcast receiver declaration that an application might include in its
     * AndroidManifest.xml to advertise custom content rating systems. The meta-data specifies a
     * resource that contains a description of each content rating system that is provided by the
     * application.
     * <p><pre class="prettyprint">
     * {@literal
     * <receiver android:name=".TvInputReceiver">
     *     <intent-filter>
     *         <action android:name=
     *                 "android.media.tv.action.QUERY_CONTENT_RATING_SYSTEMS" />
     *     </intent-filter>
     *     <meta-data
     *             android:name="android.media.tv.metadata.CONTENT_RATING_SYSTEMS"
     *             android:resource="@xml/tv_content_rating_systems" />
     * </receiver>}</pre></p>
     * In the above example, the <code>@xml/tv_content_rating_systems</code> resource refers to an
     * XML resource whose root element is <code>&lt;rating-system-definitions&gt;</code> that
     * contains zero or more <code>&lt;rating-system-definition&gt;</code> elements. Each <code>
     * &lt;rating-system-definition&gt;</code> element specifies the ratings, sub-ratings and rating
     * orders of a particular content rating system.
     * </p>
     *
     * @see TvContentRating
     */
    public static final String ACTION_QUERY_CONTENT_RATING_SYSTEMS =
            "android.media.tv.action.QUERY_CONTENT_RATING_SYSTEMS";

    /**
     * Content rating systems metadata associated with {@link #ACTION_QUERY_CONTENT_RATING_SYSTEMS}.
     * <p>
     * Specifies the resource ID of an XML resource that describes the content rating systems that
     * are provided by the application.
     * </p>
     */
    public static final String META_DATA_CONTENT_RATING_SYSTEMS =
            "android.media.tv.metadata.CONTENT_RATING_SYSTEMS";

    private final ITvInputManager mService;

    private final Object mLock = new Object();

    // @GuardedBy("mLock")
    private final List<TvInputCallbackRecord> mCallbackRecords =
            new LinkedList<TvInputCallbackRecord>();

    // A mapping from TV input ID to the state of corresponding input.
    // @GuardedBy("mLock")
    private final Map<String, Integer> mStateMap = new ArrayMap<String, Integer>();

    // A mapping from the sequence number of a session to its SessionCallbackRecord.
    private final SparseArray<SessionCallbackRecord> mSessionCallbackRecordMap =
            new SparseArray<SessionCallbackRecord>();

    // A sequence number for the next session to be created. Should be protected by a lock
    // {@code mSessionCallbackRecordMap}.
    private int mNextSeq;

    private final ITvInputClient mClient;

    private final ITvInputManagerCallback mManagerCallback;

    private final int mUserId;

    /**
     * Interface used to receive the created session.
     * @hide
     */
    @SystemApi
    public abstract static class SessionCallback {
        /**
         * This is called after {@link TvInputManager#createSession} has been processed.
         *
         * @param session A {@link TvInputManager.Session} instance created. This can be
         *            {@code null} if the creation request failed.
         */
        public void onSessionCreated(Session session) {
        }

        /**
         * This is called when {@link TvInputManager.Session} is released.
         * This typically happens when the process hosting the session has crashed or been killed.
         *
         * @param session A {@link TvInputManager.Session} instance released.
         */
        public void onSessionReleased(Session session) {
        }

        /**
         * This is called when the channel of this session is changed by the underlying TV input
         * without any {@link TvInputManager.Session#tune(Uri)} request.
         *
         * @param session A {@link TvInputManager.Session} associated with this callback.
         * @param channelUri The URI of a channel.
         */
        public void onChannelRetuned(Session session, Uri channelUri) {
        }

        /**
         * This is called when the track information of the session has been changed.
         *
         * @param session A {@link TvInputManager.Session} associated with this callback.
         * @param tracks A list which includes track information.
         */
        public void onTracksChanged(Session session, List<TvTrackInfo> tracks) {
        }

        /**
         * This is called when a track for a given type is selected.
         *
         * @param session A {@link TvInputManager.Session} associated with this callback.
         * @param type The type of the selected track. The type can be
         *            {@link TvTrackInfo#TYPE_AUDIO}, {@link TvTrackInfo#TYPE_VIDEO} or
         *            {@link TvTrackInfo#TYPE_SUBTITLE}.
         * @param trackId The ID of the selected track. When {@code null} the currently selected
         *            track for a given type should be unselected.
         */
        public void onTrackSelected(Session session, int type, String trackId) {
        }

        /**
         * This is invoked when the video size has been changed. It is also called when the first
         * time video size information becomes available after the session is tuned to a specific
         * channel.
         *
         * @param session A {@link TvInputManager.Session} associated with this callback.
         * @param width The width of the video.
         * @param height The height of the video.
         */
        public void onVideoSizeChanged(Session session, int width, int height) {
        }

        /**
         * This is called when the video is available, so the TV input starts the playback.
         *
         * @param session A {@link TvInputManager.Session} associated with this callback.
         */
        public void onVideoAvailable(Session session) {
        }

        /**
         * This is called when the video is not available, so the TV input stops the playback.
         *
         * @param session A {@link TvInputManager.Session} associated with this callback
         * @param reason The reason why the TV input stopped the playback:
         * <ul>
         * <li>{@link TvInputManager#VIDEO_UNAVAILABLE_REASON_UNKNOWN}
         * <li>{@link TvInputManager#VIDEO_UNAVAILABLE_REASON_TUNING}
         * <li>{@link TvInputManager#VIDEO_UNAVAILABLE_REASON_WEAK_SIGNAL}
         * <li>{@link TvInputManager#VIDEO_UNAVAILABLE_REASON_BUFFERING}
         * </ul>
         */
        public void onVideoUnavailable(Session session, int reason) {
        }

        /**
         * This is called when the current program content turns out to be allowed to watch since
         * its content rating is not blocked by parental controls.
         *
         * @param session A {@link TvInputManager.Session} associated with this callback
         */
        public void onContentAllowed(Session session) {
        }

        /**
         * This is called when the current program content turns out to be not allowed to watch
         * since its content rating is blocked by parental controls.
         *
         * @param session A {@link TvInputManager.Session} associated with this callback
         * @param rating The content ration of the blocked program.
         */
        public void onContentBlocked(Session session, TvContentRating rating) {
        }

        /**
         * This is called when {@link TvInputService.Session#layoutSurface} is called to change the
         * layout of surface.
         *
         * @param session A {@link TvInputManager.Session} associated with this callback
         * @param left Left position.
         * @param top Top position.
         * @param right Right position.
         * @param bottom Bottom position.
         * @hide
         */
        @SystemApi
        public void onLayoutSurface(Session session, int left, int top, int right, int bottom) {
        }

        /**
         * This is called when a custom event has been sent from this session.
         *
         * @param session A {@link TvInputManager.Session} associated with this callback
         * @param eventType The type of the event.
         * @param eventArgs Optional arguments of the event.
         * @hide
         */
        @SystemApi
        public void onSessionEvent(Session session, String eventType, Bundle eventArgs) {
        }
    }

    private static final class SessionCallbackRecord {
        private final SessionCallback mSessionCallback;
        private final Handler mHandler;
        private Session mSession;

        SessionCallbackRecord(SessionCallback sessionCallback,
                Handler handler) {
            mSessionCallback = sessionCallback;
            mHandler = handler;
        }

        void postSessionCreated(final Session session) {
            mSession = session;
            mHandler.post(new Runnable() {
                @Override
                public void run() {
                    mSessionCallback.onSessionCreated(session);
                }
            });
        }

        void postSessionReleased() {
            mHandler.post(new Runnable() {
                @Override
                public void run() {
                    mSessionCallback.onSessionReleased(mSession);
                }
            });
        }

        void postChannelRetuned(final Uri channelUri) {
            mHandler.post(new Runnable() {
                @Override
                public void run() {
                    mSessionCallback.onChannelRetuned(mSession, channelUri);
                }
            });
        }

        void postTracksChanged(final List<TvTrackInfo> tracks) {
            mHandler.post(new Runnable() {
                @Override
                public void run() {
                    mSessionCallback.onTracksChanged(mSession, tracks);
                }
            });
        }

        void postTrackSelected(final int type, final String trackId) {
            mHandler.post(new Runnable() {
                @Override
                public void run() {
                    mSessionCallback.onTrackSelected(mSession, type, trackId);
                }
            });
        }

        void postVideoSizeChanged(final int width, final int height) {
            mHandler.post(new Runnable() {
                @Override
                public void run() {
                    mSessionCallback.onVideoSizeChanged(mSession, width, height);
                }
            });
        }

        void postVideoAvailable() {
            mHandler.post(new Runnable() {
                @Override
                public void run() {
                    mSessionCallback.onVideoAvailable(mSession);
                }
            });
        }

        void postVideoUnavailable(final int reason) {
            mHandler.post(new Runnable() {
                @Override
                public void run() {
                    mSessionCallback.onVideoUnavailable(mSession, reason);
                }
            });
        }

        void postContentAllowed() {
            mHandler.post(new Runnable() {
                @Override
                public void run() {
                    mSessionCallback.onContentAllowed(mSession);
                }
            });
        }

        void postContentBlocked(final TvContentRating rating) {
            mHandler.post(new Runnable() {
                @Override
                public void run() {
                    mSessionCallback.onContentBlocked(mSession, rating);
                }
            });
        }

        void postLayoutSurface(final int left, final int top, final int right,
                final int bottom) {
            mHandler.post(new Runnable() {
                @Override
                public void run() {
                    mSessionCallback.onLayoutSurface(mSession, left, top, right, bottom);
                }
            });
        }

        void postSessionEvent(final String eventType, final Bundle eventArgs) {
            mHandler.post(new Runnable() {
                @Override
                public void run() {
                    mSessionCallback.onSessionEvent(mSession, eventType, eventArgs);
                }
            });
        }
    }

    /**
     * Callback used to monitor status of the TV input.
     */
    public abstract static class TvInputCallback {
        /**
         * This is called when the state of a given TV input is changed.
         *
         * @param inputId The id of the TV input.
         * @param state State of the TV input. The value is one of the following:
         * <ul>
         * <li>{@link TvInputManager#INPUT_STATE_CONNECTED}
         * <li>{@link TvInputManager#INPUT_STATE_CONNECTED_STANDBY}
         * <li>{@link TvInputManager#INPUT_STATE_DISCONNECTED}
         * </ul>
         */
        public void onInputStateChanged(String inputId, int state) {
        }

        /**
         * This is called when a TV input is added.
         *
         * @param inputId The id of the TV input.
         */
        public void onInputAdded(String inputId) {
        }

        /**
         * This is called when a TV input is removed.
         *
         * @param inputId The id of the TV input.
         */
        public void onInputRemoved(String inputId) {
        }

        /**
         * This is called when a TV input is updated. The update of TV input happens when it is
         * reinstalled or the media on which the newer version of TV input exists is
         * available/unavailable.
         *
         * @param inputId The id of the TV input.
         * @hide
         */
        @SystemApi
        public void onInputUpdated(String inputId) {
        }
    }

    private static final class TvInputCallbackRecord {
        private final TvInputCallback mCallback;
        private final Handler mHandler;

        public TvInputCallbackRecord(TvInputCallback callback, Handler handler) {
            mCallback = callback;
            mHandler = handler;
        }

        public TvInputCallback getCallback() {
            return mCallback;
        }

        public void postInputStateChanged(final String inputId, final int state) {
            mHandler.post(new Runnable() {
                @Override
                public void run() {
                    mCallback.onInputStateChanged(inputId, state);
                }
            });
        }

        public void postInputAdded(final String inputId) {
            mHandler.post(new Runnable() {
                @Override
                public void run() {
                    mCallback.onInputAdded(inputId);
                }
            });
        }

        public void postInputRemoved(final String inputId) {
            mHandler.post(new Runnable() {
                @Override
                public void run() {
                    mCallback.onInputRemoved(inputId);
                }
            });
        }

        public void postInputUpdated(final String inputId) {
            mHandler.post(new Runnable() {
                @Override
                public void run() {
                    mCallback.onInputUpdated(inputId);
                }
            });
        }
    }

    /**
     * Interface used to receive events from Hardware objects.
     * @hide
     */
    @SystemApi
    public abstract static class HardwareCallback {
        public abstract void onReleased();
        public abstract void onStreamConfigChanged(TvStreamConfig[] configs);
    }

    /**
     * @hide
     */
    public TvInputManager(ITvInputManager service, int userId) {
        mService = service;
        mUserId = userId;
        mClient = new ITvInputClient.Stub() {
            @Override
            public void onSessionCreated(String inputId, IBinder token, InputChannel channel,
                    int seq) {
                synchronized (mSessionCallbackRecordMap) {
                    SessionCallbackRecord record = mSessionCallbackRecordMap.get(seq);
                    if (record == null) {
                        Log.e(TAG, "Callback not found for " + token);
                        return;
                    }
                    Session session = null;
                    if (token != null) {
                        session = new Session(token, channel, mService, mUserId, seq,
                                mSessionCallbackRecordMap);
                    }
                    record.postSessionCreated(session);
                }
            }

            @Override
            public void onSessionReleased(int seq) {
                synchronized (mSessionCallbackRecordMap) {
                    SessionCallbackRecord record = mSessionCallbackRecordMap.get(seq);
                    mSessionCallbackRecordMap.delete(seq);
                    if (record == null) {
                        Log.e(TAG, "Callback not found for seq:" + seq);
                        return;
                    }
                    record.mSession.releaseInternal();
                    record.postSessionReleased();
                }
            }

            @Override
            public void onChannelRetuned(Uri channelUri, int seq) {
                synchronized (mSessionCallbackRecordMap) {
                    SessionCallbackRecord record = mSessionCallbackRecordMap.get(seq);
                    if (record == null) {
                        Log.e(TAG, "Callback not found for seq " + seq);
                        return;
                    }
                    record.postChannelRetuned(channelUri);
                }
            }

            @Override
            public void onTracksChanged(List<TvTrackInfo> tracks, int seq) {
                synchronized (mSessionCallbackRecordMap) {
                    SessionCallbackRecord record = mSessionCallbackRecordMap.get(seq);
                    if (record == null) {
                        Log.e(TAG, "Callback not found for seq " + seq);
                        return;
                    }
                    if (record.mSession.updateTracks(tracks)) {
                        record.postTracksChanged(tracks);
                        postVideoSizeChangedIfNeededLocked(record);
                    }
                }
            }

            @Override
            public void onTrackSelected(int type, String trackId, int seq) {
                synchronized (mSessionCallbackRecordMap) {
                    SessionCallbackRecord record = mSessionCallbackRecordMap.get(seq);
                    if (record == null) {
                        Log.e(TAG, "Callback not found for seq " + seq);
                        return;
                    }
                    if (record.mSession.updateTrackSelection(type, trackId)) {
                        record.postTrackSelected(type, trackId);
                        postVideoSizeChangedIfNeededLocked(record);
                    }
                }
            }

            private void postVideoSizeChangedIfNeededLocked(SessionCallbackRecord record) {
                TvTrackInfo track = record.mSession.getVideoTrackToNotify();
                if (track != null) {
                    record.postVideoSizeChanged(track.getVideoWidth(), track.getVideoHeight());
                }
            }

            @Override
            public void onVideoAvailable(int seq) {
                synchronized (mSessionCallbackRecordMap) {
                    SessionCallbackRecord record = mSessionCallbackRecordMap.get(seq);
                    if (record == null) {
                        Log.e(TAG, "Callback not found for seq " + seq);
                        return;
                    }
                    record.postVideoAvailable();
                }
            }

            @Override
            public void onVideoUnavailable(int reason, int seq) {
                synchronized (mSessionCallbackRecordMap) {
                    SessionCallbackRecord record = mSessionCallbackRecordMap.get(seq);
                    if (record == null) {
                        Log.e(TAG, "Callback not found for seq " + seq);
                        return;
                    }
                    record.postVideoUnavailable(reason);
                }
            }

            @Override
            public void onContentAllowed(int seq) {
                synchronized (mSessionCallbackRecordMap) {
                    SessionCallbackRecord record = mSessionCallbackRecordMap.get(seq);
                    if (record == null) {
                        Log.e(TAG, "Callback not found for seq " + seq);
                        return;
                    }
                    record.postContentAllowed();
                }
            }

            @Override
            public void onContentBlocked(String rating, int seq) {
                synchronized (mSessionCallbackRecordMap) {
                    SessionCallbackRecord record = mSessionCallbackRecordMap.get(seq);
                    if (record == null) {
                        Log.e(TAG, "Callback not found for seq " + seq);
                        return;
                    }
                    record.postContentBlocked(TvContentRating.unflattenFromString(rating));
                }
            }

            @Override
            public void onLayoutSurface(int left, int top, int right, int bottom, int seq) {
                synchronized (mSessionCallbackRecordMap) {
                    SessionCallbackRecord record = mSessionCallbackRecordMap.get(seq);
                    if (record == null) {
                        Log.e(TAG, "Callback not found for seq " + seq);
                        return;
                    }
                    record.postLayoutSurface(left, top, right, bottom);
                }
            }

            @Override
            public void onSessionEvent(String eventType, Bundle eventArgs, int seq) {
                synchronized (mSessionCallbackRecordMap) {
                    SessionCallbackRecord record = mSessionCallbackRecordMap.get(seq);
                    if (record == null) {
                        Log.e(TAG, "Callback not found for seq " + seq);
                        return;
                    }
                    record.postSessionEvent(eventType, eventArgs);
                }
            }
        };
        mManagerCallback = new ITvInputManagerCallback.Stub() {
            @Override
            public void onInputStateChanged(String inputId, int state) {
                synchronized (mLock) {
                    mStateMap.put(inputId, state);
                    for (TvInputCallbackRecord record : mCallbackRecords) {
                        record.postInputStateChanged(inputId, state);
                    }
                }
            }

            @Override
            public void onInputAdded(String inputId) {
                synchronized (mLock) {
                    mStateMap.put(inputId, INPUT_STATE_CONNECTED);
                    for (TvInputCallbackRecord record : mCallbackRecords) {
                        record.postInputAdded(inputId);
                    }
                }
            }

            @Override
            public void onInputRemoved(String inputId) {
                synchronized (mLock) {
                    mStateMap.remove(inputId);
                    for (TvInputCallbackRecord record : mCallbackRecords) {
                        record.postInputRemoved(inputId);
                    }
                }
            }

            @Override
            public void onInputUpdated(String inputId) {
                synchronized (mLock) {
                    for (TvInputCallbackRecord record : mCallbackRecords) {
                        record.postInputUpdated(inputId);
                    }
                }
            }
        };
        try {
            if (mService != null) {
                mService.registerCallback(mManagerCallback, mUserId);
                List<TvInputInfo> infos = mService.getTvInputList(mUserId);
                synchronized (mLock) {
                    for (TvInputInfo info : infos) {
                        String inputId = info.getId();
                        int state = mService.getTvInputState(inputId, mUserId);
                        if (state != INPUT_STATE_UNKNOWN) {
                            mStateMap.put(inputId, state);
                        }
                    }
                }
            }
        } catch (RemoteException e) {
            Log.e(TAG, "TvInputManager initialization failed: " + e);
        }
    }

    /**
     * Returns the complete list of TV inputs on the system.
     *
     * @return List of {@link TvInputInfo} for each TV input that describes its meta information.
     */
    public List<TvInputInfo> getTvInputList() {
        try {
            return mService.getTvInputList(mUserId);
        } catch (RemoteException e) {
            throw new RuntimeException(e);
        }
    }

    /**
     * Returns the {@link TvInputInfo} for a given TV input.
     *
     * @param inputId The ID of the TV input.
     * @return the {@link TvInputInfo} for a given TV input. {@code null} if not found.
     */
    public TvInputInfo getTvInputInfo(String inputId) {
        if (inputId == null) {
            throw new IllegalArgumentException("inputId cannot be null");
        }
        try {
            return mService.getTvInputInfo(inputId, mUserId);
        } catch (RemoteException e) {
            throw new RuntimeException(e);
        }
    }

    /**
     * Returns the state of a given TV input. It returns one of the following:
     * <ul>
     * <li>{@link #INPUT_STATE_CONNECTED}
     * <li>{@link #INPUT_STATE_CONNECTED_STANDBY}
     * <li>{@link #INPUT_STATE_DISCONNECTED}
     * </ul>
     *
     * @param inputId The id of the TV input.
     * @throws IllegalArgumentException if the argument is {@code null} or if there is no
     *        {@link TvInputInfo} corresponding to {@code inputId}.
     */
    public int getInputState(String inputId) {
        if (inputId == null) {
            throw new IllegalArgumentException("inputId cannot be null");
        }
        synchronized (mLock) {
            Integer state = mStateMap.get(inputId);
            if (state == null) {
                throw new IllegalArgumentException("Unrecognized input ID: " + inputId);
            }
            return state.intValue();
        }
    }

    /**
     * Registers a {@link TvInputCallback}.
     *
     * @param callback A callback used to monitor status of the TV inputs.
     * @param handler A {@link Handler} that the status change will be delivered to.
     * @throws IllegalArgumentException if any of the arguments is {@code null}.
     */
    public void registerCallback(TvInputCallback callback, Handler handler) {
        if (callback == null) {
            throw new IllegalArgumentException("callback cannot be null");
        }
        if (handler == null) {
            throw new IllegalArgumentException("handler cannot be null");
        }
        synchronized (mLock) {
            mCallbackRecords.add(new TvInputCallbackRecord(callback, handler));
        }
    }

    /**
     * Unregisters the existing {@link TvInputCallback}.
     *
     * @param callback The existing callback to remove.
     * @throws IllegalArgumentException if any of the arguments is {@code null}.
     */
    public void unregisterCallback(final TvInputCallback callback) {
        if (callback == null) {
            throw new IllegalArgumentException("callback cannot be null");
        }
        synchronized (mLock) {
            for (Iterator<TvInputCallbackRecord> it = mCallbackRecords.iterator();
                    it.hasNext(); ) {
                TvInputCallbackRecord record = it.next();
                if (record.getCallback() == callback) {
                    it.remove();
                    break;
                }
            }
        }
    }

    /**
     * Returns the user's parental controls enabled state.
     *
     * @return {@code true} if the user enabled the parental controls, {@code false} otherwise.
     */
    public boolean isParentalControlsEnabled() {
        try {
            return mService.isParentalControlsEnabled(mUserId);
        } catch (RemoteException e) {
            throw new RuntimeException(e);
        }
    }

    /**
     * Sets the user's parental controls enabled state.
     *
     * @param enabled The user's parental controls enabled state. {@code true} if the user enabled
     *            the parental controls, {@code false} otherwise.
     * @see #isParentalControlsEnabled
     * @hide
     */
    @SystemApi
    public void setParentalControlsEnabled(boolean enabled) {
        try {
            mService.setParentalControlsEnabled(enabled, mUserId);
        } catch (RemoteException e) {
            throw new RuntimeException(e);
        }
    }

    /**
     * Checks whether a given TV content rating is blocked by the user.
     *
     * @param rating The TV content rating to check.
     * @return {@code true} if the given TV content rating is blocked, {@code false} otherwise.
     */
    public boolean isRatingBlocked(TvContentRating rating) {
        if (rating == null) {
            throw new IllegalArgumentException("rating cannot be null");
        }
        try {
            return mService.isRatingBlocked(rating.flattenToString(), mUserId);
        } catch (RemoteException e) {
            throw new RuntimeException(e);
        }
    }

    /**
     * Returns the list of blocked content ratings.
     *
     * @return the list of content ratings blocked by the user.
     * @hide
     */
    @SystemApi
    public List<TvContentRating> getBlockedRatings() {
        try {
            List<TvContentRating> ratings = new ArrayList<TvContentRating>();
            for (String rating : mService.getBlockedRatings(mUserId)) {
                ratings.add(TvContentRating.unflattenFromString(rating));
            }
            return ratings;
        } catch (RemoteException e) {
            throw new RuntimeException(e);
        }
    }

    /**
     * Adds a user blocked content rating.
     *
     * @param rating The content rating to block.
     * @see #isRatingBlocked
     * @see #removeBlockedRating
     * @hide
     */
    @SystemApi
    public void addBlockedRating(TvContentRating rating) {
        if (rating == null) {
            throw new IllegalArgumentException("rating cannot be null");
        }
        try {
            mService.addBlockedRating(rating.flattenToString(), mUserId);
        } catch (RemoteException e) {
            throw new RuntimeException(e);
        }
    }

    /**
     * Removes a user blocked content rating.
     *
     * @param rating The content rating to unblock.
     * @see #isRatingBlocked
     * @see #addBlockedRating
     * @hide
     */
    @SystemApi
    public void removeBlockedRating(TvContentRating rating) {
        if (rating == null) {
            throw new IllegalArgumentException("rating cannot be null");
        }
        try {
            mService.removeBlockedRating(rating.flattenToString(), mUserId);
        } catch (RemoteException e) {
            throw new RuntimeException(e);
        }
    }

    /**
     * Returns the list of all TV content rating systems defined.
     * @hide
     */
    @SystemApi
    public List<TvContentRatingSystemInfo> getTvContentRatingSystemList() {
        try {
            return mService.getTvContentRatingSystemList(mUserId);
        } catch (RemoteException e) {
            throw new RuntimeException(e);
        }
    }

    /**
     * Creates a {@link Session} for a given TV input.
     * <p>
     * The number of sessions that can be created at the same time is limited by the capability of
     * the given TV input.
     * </p>
     *
     * @param inputId The id of the TV input.
     * @param callback A callback used to receive the created session.
     * @param handler A {@link Handler} that the session creation will be delivered to.
     * @throws IllegalArgumentException if any of the arguments is {@code null}.
     * @hide
     */
    @SystemApi
    public void createSession(String inputId, final SessionCallback callback,
            Handler handler) {
        if (inputId == null) {
            throw new IllegalArgumentException("id cannot be null");
        }
        if (callback == null) {
            throw new IllegalArgumentException("callback cannot be null");
        }
        if (handler == null) {
            throw new IllegalArgumentException("handler cannot be null");
        }
        SessionCallbackRecord record = new SessionCallbackRecord(callback, handler);
        synchronized (mSessionCallbackRecordMap) {
            int seq = mNextSeq++;
            mSessionCallbackRecordMap.put(seq, record);
            try {
                mService.createSession(mClient, inputId, seq, mUserId);
            } catch (RemoteException e) {
                throw new RuntimeException(e);
            }
        }
    }

    /**
     * Returns the TvStreamConfig list of the given TV input.
     *
     * If you are using {@link Hardware} object from {@link
     * #acquireTvInputHardware}, you should get the list of available streams
     * from {@link HardwareCallback#onStreamConfigChanged} method, not from
     * here. This method is designed to be used with {@link #captureFrame} in
     * capture scenarios specifically and not suitable for any other use.
     *
     * @param inputId the id of the TV input.
     * @return List of {@link TvStreamConfig} which is available for capturing
     *   of the given TV input.
     * @hide
     */
    @SystemApi
    public List<TvStreamConfig> getAvailableTvStreamConfigList(String inputId) {
        try {
            return mService.getAvailableTvStreamConfigList(inputId, mUserId);
        } catch (RemoteException e) {
            throw new RuntimeException(e);
        }
    }

    /**
     * Take a snapshot of the given TV input into the provided Surface.
     *
     * @param inputId the id of the TV input.
     * @param surface the {@link Surface} to which the snapshot is captured.
     * @param config the {@link TvStreamConfig} which is used for capturing.
     * @return true when the {@link Surface} is ready to be captured.
     * @hide
     */
    @SystemApi
    public boolean captureFrame(String inputId, Surface surface, TvStreamConfig config) {
        try {
            return mService.captureFrame(inputId, surface, config, mUserId);
        } catch (RemoteException e) {
            throw new RuntimeException(e);
        }
    }

    /**
     * Set the screen capture's size which should be matched with surfaceview's size
     * @hide
     */

    @SystemApi
    public void setScreenCaptureFixSize(String inputId, int width, int height) {
        try {
            mService.setScreenCaptureFixSize(inputId, width, height, mUserId);
        } catch (RemoteException e) {
            throw new RuntimeException(e);
        }
    }

    /**
     * Returns true if there is only a single TV input session.
     *
     * @hide
     */
    @SystemApi
    public boolean isSingleSessionActive() {
        try {
            return mService.isSingleSessionActive(mUserId);
        } catch (RemoteException e) {
            throw new RuntimeException(e);
        }
    }

    /**
     * Returns a list of TvInputHardwareInfo objects representing available hardware.
     *
     * @hide
     */
    @SystemApi
    public List<TvInputHardwareInfo> getHardwareList() {
        try {
            return mService.getHardwareList();
        } catch (RemoteException e) {
            throw new RuntimeException(e);
        }
    }

    /**
     * Returns acquired TvInputManager.Hardware object for given deviceId.
     *
     * If there are other Hardware object acquired for the same deviceId, calling this method will
     * preempt the previously acquired object and report {@link HardwareCallback#onReleased} to the
     * old object.
     *
     * @hide
     */
    @SystemApi
    public Hardware acquireTvInputHardware(int deviceId, final HardwareCallback callback,
            TvInputInfo info) {
        try {
            return new Hardware(
                    mService.acquireTvInputHardware(deviceId, new ITvInputHardwareCallback.Stub() {
                @Override
                public void onReleased() {
                    callback.onReleased();
                }

                @Override
                public void onStreamConfigChanged(TvStreamConfig[] configs) {
                    callback.onStreamConfigChanged(configs);
                }
            }, info, mUserId));
        } catch (RemoteException e) {
            throw new RuntimeException(e);
        }
    }

    /**
     * Releases previously acquired hardware object.
     *
     * @hide
     */
    @SystemApi
    public void releaseTvInputHardware(int deviceId, Hardware hardware) {
        try {
            mService.releaseTvInputHardware(deviceId, hardware.getInterface(), mUserId);
        } catch (RemoteException e) {
            throw new RuntimeException(e);
        }
    }

    /**
     * The Session provides the per-session functionality of TV inputs.
     * @hide
     */
    @SystemApi
    public static final class Session {
        static final int DISPATCH_IN_PROGRESS = -1;
        static final int DISPATCH_NOT_HANDLED = 0;
        static final int DISPATCH_HANDLED = 1;

        private static final long INPUT_SESSION_NOT_RESPONDING_TIMEOUT = 2500;

        private final ITvInputManager mService;
        private final int mUserId;
        private final int mSeq;

        // For scheduling input event handling on the main thread. This also serves as a lock to
        // protect pending input events and the input channel.
        private final InputEventHandler mHandler = new InputEventHandler(Looper.getMainLooper());

        private final Pool<PendingEvent> mPendingEventPool = new SimplePool<PendingEvent>(20);
        private final SparseArray<PendingEvent> mPendingEvents = new SparseArray<PendingEvent>(20);
        private final SparseArray<SessionCallbackRecord> mSessionCallbackRecordMap;

        private IBinder mToken;
        private TvInputEventSender mSender;
        private InputChannel mChannel;

        private final Object mTrackLock = new Object();
        // @GuardedBy("mTrackLock")
        private final List<TvTrackInfo> mAudioTracks = new ArrayList<TvTrackInfo>();
        // @GuardedBy("mTrackLock")
        private final List<TvTrackInfo> mVideoTracks = new ArrayList<TvTrackInfo>();
        // @GuardedBy("mTrackLock")
        private final List<TvTrackInfo> mSubtitleTracks = new ArrayList<TvTrackInfo>();
        // @GuardedBy("mTrackLock")
        private String mSelectedAudioTrackId;
        // @GuardedBy("mTrackLock")
        private String mSelectedVideoTrackId;
        // @GuardedBy("mTrackLock")
        private String mSelectedSubtitleTrackId;
        // @GuardedBy("mTrackLock")
        private int mVideoWidth;
        // @GuardedBy("mTrackLock")
        private int mVideoHeight;

        private Session(IBinder token, InputChannel channel, ITvInputManager service, int userId,
                int seq, SparseArray<SessionCallbackRecord> sessionCallbackRecordMap) {
            mToken = token;
            mChannel = channel;
            mService = service;
            mUserId = userId;
            mSeq = seq;
            mSessionCallbackRecordMap = sessionCallbackRecordMap;
        }

        /**
         * Releases this session.
         */
        public void release() {
            if (mToken == null) {
                Log.w(TAG, "The session has been already released");
                return;
            }
            try {
                mService.releaseSession(mToken, mUserId);
            } catch (RemoteException e) {
                throw new RuntimeException(e);
            }

            releaseInternal();
        }

        /**
         * Sets this as the main session. The main session is a session whose corresponding TV
         * input determines the HDMI-CEC active source device.
         *
         * @see TvView#setMain
         */
        void setMain() {
            if (mToken == null) {
                Log.w(TAG, "The session has been already released");
                return;
            }
            try {
                mService.setMainSession(mToken, mUserId);
            } catch (RemoteException e) {
                throw new RuntimeException(e);
            }
        }

        /**
         * Sets the {@link android.view.Surface} for this session.
         *
         * @param surface A {@link android.view.Surface} used to render video.
         */
        public void setSurface(Surface surface) {
            if (mToken == null) {
                Log.w(TAG, "The session has been already released");
                return;
            }
            // surface can be null.
            try {
                mService.setSurface(mToken, surface, mUserId);
            } catch (RemoteException e) {
                throw new RuntimeException(e);
            }
        }

        /**
         * Notifies of any structural changes (format or size) of the {@link Surface}
         * passed by {@link #setSurface}.
         *
         * @param format The new PixelFormat of the {@link Surface}.
         * @param width The new width of the {@link Surface}.
         * @param height The new height of the {@link Surface}.
         * @hide
         */
        @SystemApi
        public void dispatchSurfaceChanged(int format, int width, int height) {
            if (mToken == null) {
                Log.w(TAG, "The session has been already released");
                return;
            }
            try {
                mService.dispatchSurfaceChanged(mToken, format, width, height, mUserId);
            } catch (RemoteException e) {
                throw new RuntimeException(e);
            }
        }

        /**
         * Sets the relative stream volume of this session to handle a change of audio focus.
         *
         * @param volume A volume value between 0.0f to 1.0f.
         * @throws IllegalArgumentException if the volume value is out of range.
         */
        public void setStreamVolume(float volume) {
            if (mToken == null) {
                Log.w(TAG, "The session has been already released");
                return;
            }
            try {
                if (volume < 0.0f || volume > 1.0f) {
                    throw new IllegalArgumentException("volume should be between 0.0f and 1.0f");
                }
                mService.setVolume(mToken, volume, mUserId);
            } catch (RemoteException e) {
                throw new RuntimeException(e);
            }
        }

        /**
         * Tunes to a given channel.
         *
         * @param channelUri The URI of a channel.
         * @throws IllegalArgumentException if the argument is {@code null}.
         */
        public void tune(Uri channelUri) {
            tune(channelUri, null);
        }

        /**
         * Tunes to a given channel.
         *
         * @param channelUri The URI of a channel.
         * @param params A set of extra parameters which might be handled with this tune event.
         * @throws IllegalArgumentException if {@code channelUri} is {@code null}.
         * @hide
         */
        @SystemApi
        public void tune(Uri channelUri, Bundle params) {
            if (channelUri == null) {
                throw new IllegalArgumentException("channelUri cannot be null");
            }
            if (mToken == null) {
                Log.w(TAG, "The session has been already released");
                return;
            }
            synchronized (mTrackLock) {
                mAudioTracks.clear();
                mVideoTracks.clear();
                mSubtitleTracks.clear();
                mSelectedAudioTrackId = null;
                mSelectedVideoTrackId = null;
                mSelectedSubtitleTrackId = null;
                mVideoWidth = 0;
                mVideoHeight = 0;
            }
            try {
                mService.tune(mToken, channelUri, params, mUserId);
            } catch (RemoteException e) {
                throw new RuntimeException(e);
            }
        }

        /**
         * Enables or disables the caption for this session.
         *
         * @param enabled {@code true} to enable, {@code false} to disable.
         */
        public void setCaptionEnabled(boolean enabled) {
            if (mToken == null) {
                Log.w(TAG, "The session has been already released");
                return;
            }
            try {
                mService.setCaptionEnabled(mToken, enabled, mUserId);
            } catch (RemoteException e) {
                throw new RuntimeException(e);
            }
        }

        /**
         * Selects a track.
         *
         * @param type The type of the track to select. The type can be
         *            {@link TvTrackInfo#TYPE_AUDIO}, {@link TvTrackInfo#TYPE_VIDEO} or
         *            {@link TvTrackInfo#TYPE_SUBTITLE}.
         * @param trackId The ID of the track to select. When {@code null}, the currently selected
         *            track of the given type will be unselected.
         * @see #getTracks
         */
        public void selectTrack(int type, String trackId) {
            synchronized (mTrackLock) {
                if (type == TvTrackInfo.TYPE_AUDIO) {
                    if (trackId != null && !containsTrack(mAudioTracks, trackId)) {
                        Log.w(TAG, "Invalid audio trackId: " + trackId);
                        return;
                    }
                } else if (type == TvTrackInfo.TYPE_VIDEO) {
                    if (trackId != null && !containsTrack(mVideoTracks, trackId)) {
                        Log.w(TAG, "Invalid video trackId: " + trackId);
                        return;
                    }
                } else if (type == TvTrackInfo.TYPE_SUBTITLE) {
                    if (trackId != null && !containsTrack(mSubtitleTracks, trackId)) {
                        Log.w(TAG, "Invalid subtitle trackId: " + trackId);
                        return;
                    }
                } else {
                    throw new IllegalArgumentException("invalid type: " + type);
                }
            }
            if (mToken == null) {
                Log.w(TAG, "The session has been already released");
                return;
            }
            try {
                mService.selectTrack(mToken, type, trackId, mUserId);
            } catch (RemoteException e) {
                throw new RuntimeException(e);
            }
        }

        private boolean containsTrack(List<TvTrackInfo> tracks, String trackId) {
            for (TvTrackInfo track : tracks) {
                if (track.getId().equals(trackId)) {
                    return true;
                }
            }
            return false;
        }

        /**
         * Returns the list of tracks for a given type. Returns {@code null} if the information is
         * not available.
         *
         * @param type The type of the tracks. The type can be {@link TvTrackInfo#TYPE_AUDIO},
         *            {@link TvTrackInfo#TYPE_VIDEO} or {@link TvTrackInfo#TYPE_SUBTITLE}.
         * @return the list of tracks for the given type.
         */
        public List<TvTrackInfo> getTracks(int type) {
            synchronized (mTrackLock) {
                if (type == TvTrackInfo.TYPE_AUDIO) {
                    if (mAudioTracks == null) {
                        return null;
                    }
                    return new ArrayList<TvTrackInfo>(mAudioTracks);
                } else if (type == TvTrackInfo.TYPE_VIDEO) {
                    if (mVideoTracks == null) {
                        return null;
                    }
                    return new ArrayList<TvTrackInfo>(mVideoTracks);
                } else if (type == TvTrackInfo.TYPE_SUBTITLE) {
                    if (mSubtitleTracks == null) {
                        return null;
                    }
                    return new ArrayList<TvTrackInfo>(mSubtitleTracks);
                }
            }
            throw new IllegalArgumentException("invalid type: " + type);
        }

        /**
         * Returns the selected track for a given type. Returns {@code null} if the information is
         * not available or any of the tracks for the given type is not selected.
         *
         * @return the ID of the selected track.
         * @see #selectTrack
         */
        public String getSelectedTrack(int type) {
            synchronized (mTrackLock) {
                if (type == TvTrackInfo.TYPE_AUDIO) {
                    return mSelectedAudioTrackId;
                } else if (type == TvTrackInfo.TYPE_VIDEO) {
                    return mSelectedVideoTrackId;
                } else if (type == TvTrackInfo.TYPE_SUBTITLE) {
                    return mSelectedSubtitleTrackId;
                }
            }
            throw new IllegalArgumentException("invalid type: " + type);
        }

        /**
         * Responds to onTracksChanged() and updates the internal track information. Returns true if
         * there is an update.
         */
        boolean updateTracks(List<TvTrackInfo> tracks) {
            synchronized (mTrackLock) {
                mAudioTracks.clear();
                mVideoTracks.clear();
                mSubtitleTracks.clear();
                for (TvTrackInfo track : tracks) {
                    if (track.getType() == TvTrackInfo.TYPE_AUDIO) {
                        mAudioTracks.add(track);
                    } else if (track.getType() == TvTrackInfo.TYPE_VIDEO) {
                        mVideoTracks.add(track);
                    } else if (track.getType() == TvTrackInfo.TYPE_SUBTITLE) {
                        mSubtitleTracks.add(track);
                    }
                }
                return !mAudioTracks.isEmpty() || !mVideoTracks.isEmpty()
                        || !mSubtitleTracks.isEmpty();
            }
        }

        /**
         * Responds to onTrackSelected() and updates the internal track selection information.
         * Returns true if there is an update.
         */
        boolean updateTrackSelection(int type, String trackId) {
            synchronized (mTrackLock) {
                if (type == TvTrackInfo.TYPE_AUDIO && trackId != mSelectedAudioTrackId) {
                    mSelectedAudioTrackId = trackId;
                    return true;
                } else if (type == TvTrackInfo.TYPE_VIDEO && trackId != mSelectedVideoTrackId) {
                    mSelectedVideoTrackId = trackId;
                    return true;
                } else if (type == TvTrackInfo.TYPE_SUBTITLE
                        && trackId != mSelectedSubtitleTrackId) {
                    mSelectedSubtitleTrackId = trackId;
                    return true;
                }
            }
            return false;
        }

        /**
         * Returns the new/updated video track that contains new video size information. Returns
         * null if there is no video track to notify. Subsequent calls of this method results in a
         * non-null video track returned only by the first call and null returned by following
         * calls. The caller should immediately notify of the video size change upon receiving the
         * track.
         */
        TvTrackInfo getVideoTrackToNotify() {
            synchronized (mTrackLock) {
                if (!mVideoTracks.isEmpty() && mSelectedVideoTrackId != null) {
                    for (TvTrackInfo track : mVideoTracks) {
                        if (track.getId().equals(mSelectedVideoTrackId)) {
                            int videoWidth = track.getVideoWidth();
                            int videoHeight = track.getVideoHeight();
                            if (mVideoWidth != videoWidth || mVideoHeight != videoHeight) {
                                mVideoWidth = videoWidth;
                                mVideoHeight = videoHeight;
                                return track;
                            }
                        }
                    }
                }
            }
            return null;
        }

        /**
         * Calls {@link TvInputService.Session#appPrivateCommand(String, Bundle)
         * TvInputService.Session.appPrivateCommand()} on the current TvView.
         *
         * @param action Name of the command to be performed. This <em>must</em> be a scoped name,
         *            i.e. prefixed with a package name you own, so that different developers will
         *            not create conflicting commands.
         * @param data Any data to include with the command.
         * @hide
         */
        @SystemApi
        public void sendAppPrivateCommand(String action, Bundle data) {
            if (mToken == null) {
                Log.w(TAG, "The session has been already released");
                return;
            }
            try {
                mService.sendAppPrivateCommand(mToken, action, data, mUserId);
            } catch (RemoteException e) {
                throw new RuntimeException(e);
            }
        }

        /**
         * Creates an overlay view. Once the overlay view is created, {@link #relayoutOverlayView}
         * should be called whenever the layout of its containing view is changed.
         * {@link #removeOverlayView()} should be called to remove the overlay view.
         * Since a session can have only one overlay view, this method should be called only once
         * or it can be called again after calling {@link #removeOverlayView()}.
         *
         * @param view A view playing TV.
         * @param frame A position of the overlay view.
         * @throws IllegalArgumentException if any of the arguments is {@code null}.
         * @throws IllegalStateException if {@code view} is not attached to a window.
         */
        void createOverlayView(View view, Rect frame) {
            if (view == null) {
                throw new IllegalArgumentException("view cannot be null");
            }
            if (frame == null) {
                throw new IllegalArgumentException("frame cannot be null");
            }
            if (view.getWindowToken() == null) {
                throw new IllegalStateException("view must be attached to a window");
            }
            if (mToken == null) {
                Log.w(TAG, "The session has been already released");
                return;
            }
            try {
                mService.createOverlayView(mToken, view.getWindowToken(), frame, mUserId);
            } catch (RemoteException e) {
                throw new RuntimeException(e);
            }
        }

        /**
         * Relayouts the current overlay view.
         *
         * @param frame A new position of the overlay view.
         * @throws IllegalArgumentException if the arguments is {@code null}.
         */
        void relayoutOverlayView(Rect frame) {
            if (frame == null) {
                throw new IllegalArgumentException("frame cannot be null");
            }
            if (mToken == null) {
                Log.w(TAG, "The session has been already released");
                return;
            }
            try {
                mService.relayoutOverlayView(mToken, frame, mUserId);
            } catch (RemoteException e) {
                throw new RuntimeException(e);
            }
        }

        /**
         * Removes the current overlay view.
         */
        void removeOverlayView() {
            if (mToken == null) {
                Log.w(TAG, "The session has been already released");
                return;
            }
            try {
                mService.removeOverlayView(mToken, mUserId);
            } catch (RemoteException e) {
                throw new RuntimeException(e);
            }
        }

        /**
         * Requests to unblock content blocked by parental controls.
         */
        void requestUnblockContent(TvContentRating unblockedRating) {
            if (mToken == null) {
                Log.w(TAG, "The session has been already released");
                return;
            }
            if (unblockedRating == null) {
                throw new IllegalArgumentException("unblockedRating cannot be null");
            }
            try {
                mService.requestUnblockContent(mToken, unblockedRating.flattenToString(), mUserId);
            } catch (RemoteException e) {
                throw new RuntimeException(e);
            }
        }

        /**
         * Dispatches an input event to this session.
         *
         * @param event An {@link InputEvent} to dispatch.
         * @param token A token used to identify the input event later in the callback.
         * @param callback A callback used to receive the dispatch result.
         * @param handler A {@link Handler} that the dispatch result will be delivered to.
         * @return Returns {@link #DISPATCH_HANDLED} if the event was handled. Returns
         *         {@link #DISPATCH_NOT_HANDLED} if the event was not handled. Returns
         *         {@link #DISPATCH_IN_PROGRESS} if the event is in progress and the callback will
         *         be invoked later.
         * @throws IllegalArgumentException if any of the necessary arguments is {@code null}.
         * @hide
         */
        public int dispatchInputEvent(InputEvent event, Object token,
                FinishedInputEventCallback callback, Handler handler) {
            if (event == null) {
                throw new IllegalArgumentException("event cannot be null");
            }
            if (callback != null && handler == null) {
                throw new IllegalArgumentException("handler cannot be null");
            }
            synchronized (mHandler) {
                if (mChannel == null) {
                    return DISPATCH_NOT_HANDLED;
                }
                PendingEvent p = obtainPendingEventLocked(event, token, callback, handler);
                if (Looper.myLooper() == Looper.getMainLooper()) {
                    // Already running on the main thread so we can send the event immediately.
                    return sendInputEventOnMainLooperLocked(p);
                }

                // Post the event to the main thread.
                Message msg = mHandler.obtainMessage(InputEventHandler.MSG_SEND_INPUT_EVENT, p);
                msg.setAsynchronous(true);
                mHandler.sendMessage(msg);
                return DISPATCH_IN_PROGRESS;
            }
        }

        /**
         * Callback that is invoked when an input event that was dispatched to this session has been
         * finished.
         *
         * @hide
         */
        public interface FinishedInputEventCallback {
            /**
             * Called when the dispatched input event is finished.
             *
             * @param token A token passed to {@link #dispatchInputEvent}.
             * @param handled {@code true} if the dispatched input event was handled properly.
             *            {@code false} otherwise.
             */
            public void onFinishedInputEvent(Object token, boolean handled);
        }

        // Must be called on the main looper
        private void sendInputEventAndReportResultOnMainLooper(PendingEvent p) {
            synchronized (mHandler) {
                int result = sendInputEventOnMainLooperLocked(p);
                if (result == DISPATCH_IN_PROGRESS) {
                    return;
                }
            }

            invokeFinishedInputEventCallback(p, false);
        }

        private int sendInputEventOnMainLooperLocked(PendingEvent p) {
            if (mChannel != null) {
                if (mSender == null) {
                    mSender = new TvInputEventSender(mChannel, mHandler.getLooper());
                }

                final InputEvent event = p.mEvent;
                final int seq = event.getSequenceNumber();
                if (mSender.sendInputEvent(seq, event)) {
                    mPendingEvents.put(seq, p);
                    Message msg = mHandler.obtainMessage(InputEventHandler.MSG_TIMEOUT_INPUT_EVENT, p);
                    msg.setAsynchronous(true);
                    mHandler.sendMessageDelayed(msg, INPUT_SESSION_NOT_RESPONDING_TIMEOUT);
                    return DISPATCH_IN_PROGRESS;
                }

                Log.w(TAG, "Unable to send input event to session: " + mToken + " dropping:"
                        + event);
            }
            return DISPATCH_NOT_HANDLED;
        }

        void finishedInputEvent(int seq, boolean handled, boolean timeout) {
            final PendingEvent p;
            synchronized (mHandler) {
                int index = mPendingEvents.indexOfKey(seq);
                if (index < 0) {
                    return; // spurious, event already finished or timed out
                }

                p = mPendingEvents.valueAt(index);
                mPendingEvents.removeAt(index);

                if (timeout) {
                    Log.w(TAG, "Timeout waiting for seesion to handle input event after "
                            + INPUT_SESSION_NOT_RESPONDING_TIMEOUT + " ms: " + mToken);
                } else {
                    mHandler.removeMessages(InputEventHandler.MSG_TIMEOUT_INPUT_EVENT, p);
                }
            }

            invokeFinishedInputEventCallback(p, handled);
        }

        // Assumes the event has already been removed from the queue.
        void invokeFinishedInputEventCallback(PendingEvent p, boolean handled) {
            p.mHandled = handled;
            if (p.mEventHandler.getLooper().isCurrentThread()) {
                // Already running on the callback handler thread so we can send the callback
                // immediately.
                p.run();
            } else {
                // Post the event to the callback handler thread.
                // In this case, the callback will be responsible for recycling the event.
                Message msg = Message.obtain(p.mEventHandler, p);
                msg.setAsynchronous(true);
                msg.sendToTarget();
            }
        }

        private void flushPendingEventsLocked() {
            mHandler.removeMessages(InputEventHandler.MSG_FLUSH_INPUT_EVENT);

            final int count = mPendingEvents.size();
            for (int i = 0; i < count; i++) {
                int seq = mPendingEvents.keyAt(i);
                Message msg = mHandler.obtainMessage(InputEventHandler.MSG_FLUSH_INPUT_EVENT, seq, 0);
                msg.setAsynchronous(true);
                msg.sendToTarget();
            }
        }

        private PendingEvent obtainPendingEventLocked(InputEvent event, Object token,
                FinishedInputEventCallback callback, Handler handler) {
            PendingEvent p = mPendingEventPool.acquire();
            if (p == null) {
                p = new PendingEvent();
            }
            p.mEvent = event;
            p.mEventToken = token;
            p.mCallback = callback;
            p.mEventHandler = handler;
            return p;
        }

        private void recyclePendingEventLocked(PendingEvent p) {
            p.recycle();
            mPendingEventPool.release(p);
        }

        IBinder getToken() {
            return mToken;
        }

        private void releaseInternal() {
            mToken = null;
            synchronized (mHandler) {
                if (mChannel != null) {
                    if (mSender != null) {
                        flushPendingEventsLocked();
                        mSender.dispose();
                        mSender = null;
                    }
                    mChannel.dispose();
                    mChannel = null;
                }
            }
            synchronized (mSessionCallbackRecordMap) {
                mSessionCallbackRecordMap.remove(mSeq);
            }
        }

        private final class InputEventHandler extends Handler {
            public static final int MSG_SEND_INPUT_EVENT = 1;
            public static final int MSG_TIMEOUT_INPUT_EVENT = 2;
            public static final int MSG_FLUSH_INPUT_EVENT = 3;

            InputEventHandler(Looper looper) {
                super(looper, null, true);
            }

            @Override
            public void handleMessage(Message msg) {
                switch (msg.what) {
                    case MSG_SEND_INPUT_EVENT: {
                        sendInputEventAndReportResultOnMainLooper((PendingEvent) msg.obj);
                        return;
                    }
                    case MSG_TIMEOUT_INPUT_EVENT: {
                        finishedInputEvent(msg.arg1, false, true);
                        return;
                    }
                    case MSG_FLUSH_INPUT_EVENT: {
                        finishedInputEvent(msg.arg1, false, false);
                        return;
                    }
                }
            }
        }

        private final class TvInputEventSender extends InputEventSender {
            public TvInputEventSender(InputChannel inputChannel, Looper looper) {
                super(inputChannel, looper);
            }

            @Override
            public void onInputEventFinished(int seq, boolean handled) {
                finishedInputEvent(seq, handled, false);
            }
        }

        private final class PendingEvent implements Runnable {
            public InputEvent mEvent;
            public Object mEventToken;
            public FinishedInputEventCallback mCallback;
            public Handler mEventHandler;
            public boolean mHandled;

            public void recycle() {
                mEvent = null;
                mEventToken = null;
                mCallback = null;
                mEventHandler = null;
                mHandled = false;
            }

            @Override
            public void run() {
                mCallback.onFinishedInputEvent(mEventToken, mHandled);

                synchronized (mEventHandler) {
                    recyclePendingEventLocked(this);
                }
            }
        }
    }

    /**
     * The Hardware provides the per-hardware functionality of TV hardware.
     *
     * TV hardware is physical hardware attached to the Android device; for example, HDMI ports,
     * Component/Composite ports, etc. Specifically, logical devices such as HDMI CEC logical
     * devices don't fall into this category.
     *
     * @hide
     */
    @SystemApi
    public final static class Hardware {
        private final ITvInputHardware mInterface;

        private Hardware(ITvInputHardware hardwareInterface) {
            mInterface = hardwareInterface;
        }

        private ITvInputHardware getInterface() {
            return mInterface;
        }

        public boolean setSurface(Surface surface, TvStreamConfig config) {
            try {
                return mInterface.setSurface(surface, config);
            } catch (RemoteException e) {
                throw new RuntimeException(e);
            }
        }

        public void setStreamVolume(float volume) {
            try {
                mInterface.setStreamVolume(volume);
            } catch (RemoteException e) {
                throw new RuntimeException(e);
            }
        }

        public boolean dispatchKeyEventToHdmi(KeyEvent event) {
            try {
                return mInterface.dispatchKeyEventToHdmi(event);
            } catch (RemoteException e) {
                throw new RuntimeException(e);
            }
        }

        public void overrideAudioSink(int audioType, String audioAddress, int samplingRate,
                int channelMask, int format) {
            try {
                mInterface.overrideAudioSink(audioType, audioAddress, samplingRate, channelMask,
                        format);
            } catch (RemoteException e) {
                throw new RuntimeException(e);
            }
        }
    }
}
